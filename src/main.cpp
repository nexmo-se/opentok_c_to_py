#include <opentok.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <signal.h>

#include "config.h"
#include "otk_thread.h"
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

long int begin_timestamp = 0;
int frame_count = 0;
struct timeval tp;


static std::atomic<bool> g_is_connected(false);

//Define our video output file
FILE *video_out_file;
char *video_resolution = "640x480";
char *video_output_name = "/tmp/vonage_frame_buffer.fifo"; //our named pipe
//store ffmpeg command here
char command[256];


//CREATE A CUSTOM AUDIO DEVICE (SKIP IF YOU DO NOT NEED ONE)
struct audio_device {
  otc_audio_device_callbacks audio_device_callbacks;
  otk_thread_t renderer_thread;
  std::atomic<bool> renderer_thread_exit;
};

static otk_thread_func_return_type renderer_thread_start_function(void *arg) {
  struct audio_device *device = static_cast<struct audio_device *>(arg);
  if (device == nullptr) {
    otk_thread_func_return_value;
  }

  while (device->renderer_thread_exit.load() == false) {
  	int16_t samples[480];
    size_t actual = otc_audio_device_read_render_data(samples,480);
	  usleep(9807);
  }
  otk_thread_func_return_value;
}

/*||||CALLBACKS FOR SUBSCRIBER EVENTS||||*/
static void on_subscriber_connected(otc_subscriber *subscriber,
                                    void *user_data,
                                    const otc_stream *stream) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_subscriber_error(otc_subscriber* subscriber,
                                void *user_data,
                                const char* error_string,
                                enum otc_subscriber_error_code error) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "Subscriber error. Error code: " << error_string << std::endl;
}

//Receive Subscriber Video Frames Here (FRAMES ARE IN YUV FORMAT)
static void on_subscriber_render_frame(otc_subscriber *subscriber, 
                        void *user_data,
                        const otc_video_frame *frame){

  int width = otc_video_frame_get_width(frame); //get width of the frame
  int height = otc_video_frame_get_height(frame); //get height of the frame

  uint8_t* buffer = (uint8_t*)otc_video_frame_get_buffer(frame); //get the frame buffer
  size_t buffer_size = otc_video_frame_get_buffer_size(frame); //get the size of the frame buffer

  //In this section, you can send the frame to your destination of choice
  //Here we are writing it to ffmpeg linked file
  fwrite(buffer, 1,
        buffer_size,video_out_file);
  
}

static void on_subscriber_audio_data(otc_subscriber* subscriber,
                        void* user_data,
                        const struct otc_audio_data* audio_data){
  //Add Implementation here

}
/*||||END CALLBACKS FOR SUBSCRIBER EVENTS||||*/


/*****************CALLBACKS FOR AUDIO DEVICE EVENTS (ONLY WHEN USING CUSTOM AUDIO DEVICE)*****************/
static otc_bool audio_device_destroy_renderer(const otc_audio_device *audio_device,
                                              void *user_data) {
  struct audio_device *device = static_cast<struct audio_device *>(user_data);
  if (device == nullptr) {
    return OTC_FALSE;
  }

  device->renderer_thread_exit = true;
  otk_thread_join(device->renderer_thread);

  return OTC_TRUE;
}

static otc_bool audio_device_start_renderer(const otc_audio_device *audio_device,
                                            void *user_data) {
  struct audio_device *device = static_cast<struct audio_device *>(user_data);
  printf("Starting audio renderer\n");
  if (device == nullptr) {
    return OTC_FALSE;
  }

  device->renderer_thread_exit = false;
  if (otk_thread_create(&(device->renderer_thread), &renderer_thread_start_function, (void *)device) != 0) {
    return OTC_FALSE;
  }
  printf("Started audio renderer\n");
  return OTC_TRUE;
}

static otc_bool audio_device_get_render_settings(const otc_audio_device *audio_device,
                                                  void *user_data,
                                                  struct otc_audio_device_settings *settings) {
  if (settings == nullptr) {
    return OTC_FALSE;
  }

  settings->number_of_channels = 1;
  settings->sampling_rate = 48000;
  return OTC_TRUE;
}
/*****************END CALLBACKS FOR AUDIO DEVICE EVENTS*****************/

/*****************CALLBACKS FOR SESSION EVENTS*****************/
static void on_session_connected(otc_session *session, void *user_data) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
  g_is_connected = true;
}

static void on_session_connection_created(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_connection_dropped(otc_session *session,
                                          void *user_data,
                                          const otc_connection *connection) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_stream_dropped(otc_session *session,
                                      void *user_data,
                                      const otc_stream *stream) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_disconnected(otc_session *session, void *user_data) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
}

static void on_session_error(otc_session *session,
                             void *user_data,
                             const char *error_string,
                             enum otc_session_error_code error) {
  std::cout << __FUNCTION__ << " callback function" << std::endl;
  std::cout << "Session error. Error : " << error_string << std::endl;
}

static void on_session_stream_received(otc_session *session,
                                       void *user_data,
                                       const otc_stream *stream) {

  //let's set the beginning time for our elapsed time counter for fps
  gettimeofday(&tp, NULL);
  begin_timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;

  std::cout << __FUNCTION__ << " callback function" << std::endl;

  //DEFINE SUBSCRIBER RELATED CALLBACKS
  //SEE CALLBACKS FOR SUBSCRIBER EVENTS SECTION
  struct otc_subscriber_callbacks subscriber_callbacks = {0};
  subscriber_callbacks.user_data = user_data;
  subscriber_callbacks.on_connected = on_subscriber_connected;
  subscriber_callbacks.on_render_frame = on_subscriber_render_frame;
  subscriber_callbacks.on_error = on_subscriber_error;
  subscriber_callbacks.on_audio_data = on_subscriber_audio_data;

  otc_subscriber *subscriber = otc_subscriber_new(stream,&subscriber_callbacks);
  //otc_subscriber_set_subscribe_to_video(subscriber,1);
 
  if (otc_session_subscribe(session, subscriber) == OTC_SUCCESS) {
    printf("subscribed successfully\n");
    return;
  }
  else{
    printf("Error during subscribe\n");
  }
}


/*****************END CALLBACKS FOR SESSION EVENTS*****************/


static void on_otc_log_message(const char* message) {
  std::cout <<  __FUNCTION__ << ":" << message << std::endl;
}

void sigfun(int sig)
{
        printf("You have presses Ctrl-C , please press again to exit\n");
	(void) signal(SIGINT, SIG_DFL);
}

int main(int argc, char** argv) {
//let's create a named Pipe where the Python OpenCV2 will read frames
  mkfifo(video_output_name, 0777);
  printf("Value of errno: %d\n ", errno); //if this is 17 then file already exists, that;s okay
  printf("The error message is : %s\n", strerror(errno));

//CHECK if OPENTOK LIBRARY CAN BE FOUND
  if (otc_init(nullptr) != OTC_SUCCESS) {
    std::cout << "Could not init OpenTok library" << std::endl;
    return EXIT_FAILURE;
  }
  (void) signal(SIGINT, sigfun);
 
#ifdef CONSOLE_LOGGING
  otc_log_set_logger_callback(on_otc_log_message);
  otc_log_enable(OTC_LOG_LEVEL_ALL);
#endif


  //Create a pipe to ffmpeg so out YUV frames gets converted to mpeg4 right away
  //ON Production, make sure resolution (-s 640x480) is handled correctly, 
  //if it is dynamic, on change, you need to restart ffmpeg and save to another file or append to created file (needs to be same output file)

  sprintf(command, "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt yuv420p -s %s -r 25 -i - -f mpeg -q:v 5 -an -vf scale=1024x768 -vcodec mpeg4 pipe:1 > %s", video_resolution, video_output_name);
  video_out_file = popen(command, "w");
  std::cout<<"ffmpeg" << command<<std::endl;
  
// ffmpeg -i video_floor.mp4 -f mpeg pipe:1 > fifo.avi
  //DEFINE CUSTOM AUDIO DEVICE RELATED CALLBACKS (SKIP IF NOT NEEDED)
  //SEE CALLBACKS FOR AUDIO DEVICE EVENTS SECTION
  struct audio_device *device = (struct audio_device *)malloc(sizeof(struct audio_device));
  device->audio_device_callbacks = {0};
  device->audio_device_callbacks.user_data = static_cast<void *>(device);
  device->audio_device_callbacks.destroy_renderer = audio_device_destroy_renderer;
  device->audio_device_callbacks.start_renderer = audio_device_start_renderer;
  device->audio_device_callbacks.get_render_settings = audio_device_get_render_settings;
  otc_set_audio_device(&(device->audio_device_callbacks));
  
  //DEFINE SESSION RELATED CALLBACKS
  //SEE CALLBACKS FOR SESSION EVENTS SECTION
  struct otc_session_callbacks session_callbacks = {0};
  session_callbacks.on_connected = on_session_connected;
  session_callbacks.on_connection_created = on_session_connection_created;
  session_callbacks.on_connection_dropped = on_session_connection_dropped;
  session_callbacks.on_stream_received = on_session_stream_received;
  session_callbacks.on_stream_dropped = on_session_stream_dropped;
  session_callbacks.on_disconnected = on_session_disconnected;
  session_callbacks.on_error = on_session_error;
  
  //CONNECT TO A SESSION
  otc_session *session = nullptr;
  session = otc_session_new(API_KEY, SESSION_ID, &session_callbacks);

  if (session == nullptr) {
    std::cout << "Could not create OpenTok session successfully" << std::endl;
    return EXIT_FAILURE;
  }

  otc_session_connect(session, TOKEN);


  while(1){
	  sleep(1);
  }

  if ((session != nullptr) && g_is_connected.load()) {
    otc_session_disconnect(session);
  }

  if (session != nullptr) {
    otc_session_delete(session);
  }

  if (device != nullptr) {
    free(device);
  }

  otc_destroy();

  return EXIT_SUCCESS;
}
