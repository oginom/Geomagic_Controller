#!/usr/bin/env python3

import socket
import time


class GeomagicManager:
  def __init__(self, port=54321, ip_address='localhost'):
    self.port = port
    self.ip_address = ip_address
    self.client = None

  def __enter__(self):
    self.Init()
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    self.Close()

  def Init(self):
    self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.client.connect((self.ip_address, self.port))

  def Send(self, content):
    self.client.send(content.encode('utf-8'))

  def Receive(self):
    response = self.client.recv(4096)
    return response.decode()

  def Close(self):
    self.client.close()

  def SendData(self, data):
    self.Send(','.join([str(x) for x in data]))

  def ReceiveData(self):
    data = self.Receive().split(',')
    return data

  def GetPosition(self):
    self.Send('p')
    r = self.Receive()
    return [float(x) for x in r.split(',')]

  def SetTarget(self, target):
    self.Send('t %d,%d,%d' % (target[0], target[1], target[2]))
    r = self.Receive()
    if r != 'ok':
      print(r)

  def GetButtons(self):
    self.Send('b')
    r = self.Receive()
    n = int(r)
    return (n & 1, n >> 1 & 1)

if __name__ == '__main__':
  with GeomagicManager() as gm:
    while True:
      a = input('send : ')
      if a == 'quit':
        break
      elif a == '':
        continue
      else:
        gm.Send(a)
        print('recv : %s' % gm.Receive())
