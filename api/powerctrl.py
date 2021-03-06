import serial
import threading
import Queue
import time

class SerialActor():

    def __init__(self, ser_device, baud=9600):
        self.ser = serial.Serial(port=ser_device, baudrate=baud)
        # Arduinos need time for serial response .. 
        time.sleep(2)

    def send(self, datagram):

        try:
            self.ser.write(datagram)

            msg = ""
            while True:
                msg += self.ser.read(1)
                if "OK" in msg:
                    break


        except:
            if self.ser.isOpen():
                self.ser.close()

            self.ser.open()
            time.sleep(2)

            # try again
            self.send(datagram)

    def switch(self, port, on):
        switch = 1 if on else 0
        self.send('sp%d.%d\r' % (port, switch))


class DecoupledActor(threading.Thread):

    def __init__(self, actor):
        self.actor = actor
        self.queue = Queue.Queue()

        super(DecoupledActor, self).__init__()

    def switch(self, port, on):
        self.queue.put((port, on))

    def run(self):

        while True:

            port, on = self.queue.get()
            self.actor.switch(port, on)

            self.queue.task_done()
            time.sleep(0.1)

class Device:

    def __init__(self, actor, config):
        self.actor = actor
        self.name = config['name']
        self.port = int(config['config']['port'])
        self.status = False

    def get_status(self):
        return self.status

    def set_status(self, on):
        self.actor.switch(self.port, on)
        self.status = on

    def serialize(self):
        return {
            'status': self.status,
            'name': self.name
        }
