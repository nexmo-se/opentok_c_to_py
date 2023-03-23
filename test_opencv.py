import signal
import cv2

cap = cv2.VideoCapture(r"/tmp/vonage_frame_buffer.fifo") #this is our pipe file
width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH) + 0.5)
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT) + 0.5)
size = (width, height)
fourcc = cv2.VideoWriter_fourcc('M','J','P','G')
out = cv2.VideoWriter('output.avi', fourcc, 20.0, size, 0)

results = {}
curr_frame = 0
while(cap.isOpened()):
    # Capture frame-by-frame
    try:
      ret, frame = cap.read()
    except KeyboardInterrupt:
      print("CTRL C received. Ending loop");
      break
    if ret == True:
      curr_frame+=1
      frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
      out.write(frame)
      print(frame)
      if cv2.waitKey(1) & 0xFF == ord('q'):
        break

def ctrlC_handler(signum, frame):
  print("CTRL C received. Ending");
  # do not do cap.release() since this is a fifo pipe.
  # If you close the pipe, the C program cannot write to it anymore
  out.release()

signal.signal(signal.SIGINT, ctrlC_handler)

