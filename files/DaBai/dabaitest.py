import serial
import os
import time

# open serial port /dev/ttyS1
bt_ser = serial.Serial()
bt_ser.port = "/dev/ttyS0"
bt_ser.baudrate = 57600
bt_ser.bytesize = serial.EIGHTBITS
bt_ser.parity = serial.PARITY_NONE
bt_ser.stopbits = serial.STOPBITS_ONE
bt_ser.xonxoff = False
bt_ser.rtscts = False
bt_ser.dsrtdr = False
bt_ser.timeout = 1
bt_ser.writeTimeou = 2

try:
  bt_ser.open()
except Exception, e :
  print "open serial port error : " + str(e)
  exit()

# check serial was opend
if bt_ser.isOpen() :
  print("Serial port is open")
  data_byte = ''
  data_count = 0
  present_time = time.time()
  current_time = present_time
  while True :
    current_time = time.time()
    while bt_ser.inWaiting() :
      data_byte += bt_ser.read(1)
      data_count += 1
      present_time = current_time

    if (current_time - present_time > 0.01) :
      if data_count != 0 :
        document = open("/IoT/dabai/test.json", "w+")
        write_start_time = time.time()
        document.write(data_byte)
        write_spend_time = time.time() - write_start_time
        document.close()
        print("Receive Count : %d" % data_count)
        data_byte = 0
        data_count = 0
        send_doc = open("/IoT/dabai/test.json", "r")
        context = send_doc.read()
        start_send_time = time.time()
        send_count = bt_ser.write(context)
        spend_time = time.time() - start_send_time
        print("Send byte : %d" % send_count)
        print("Spend time : %d" % spend_time)
        send_doc.close()
      present_time = current_time
