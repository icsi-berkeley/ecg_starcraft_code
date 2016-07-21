"""
This file contains the functions that interact directly with the starcraft API and code
running on a separate machine.
"""

from threading import Thread, Lock
from functools import wraps
from nluas.Transport import Transport
import json
import time


def run_async(func):
    @wraps(func)
    def async_func(*args, **kwargs):
        func_process = Thread(target=func, args=args, kwargs=kwargs)
        func_process.start()
        return func_process
    return async_func


class Starcraft_Game(object):

    def __init__(self, transport):
        self.game_address = "StarCraft"
        self.transport = transport
        self.transport.subscribe(self.game_address, self.callback)

        self._lock = Lock()
        self._responses = []
        self._completed = []

    @run_async
    def callback(self, response, *args, **kwargs):
        """
        Called asychronously when a response is sent from the game. Adds it to a
        queue of game responses.
        """
        with self._lock:
            self._responses.append(json.loads(json.loads(response)))

    def send_and_receive(self, message, timeout=5):
        """
        Sends message and waits for a response with response_head
        """
        self._completed.sort()
        self._completed.reverse()
        with self._lock:
            for index in self._completed:
                del self._responses[index]
        self._completed = []

        send_time = time.time()
        response_head = str(send_time)
        message["response_head"] = response_head
        self.transport.send(self.game_address, json.dumps(message))
        print("sent: ", message)

        while time.time() - send_time <= timeout:
            for index, response in enumerate(self._responses):
                if isinstance(response, dict) and 'head' in response and response['head'] == response_head:
                    print("received: ", response)
                    self._completed.insert(0, index)
                    if response["status"] == "success" or 'remaining' in response:
                        return response
                    raise RuntimeWarning("Could not complete: " + str(message))
        raise RuntimeError("Command timed out: " + str(message))

    def get_worker(self):
        """
        Returns a unit object for a worker in the game.
        """
        #response = self.send_and_receive({"action": "get_worker"})
        response = self.send_and_receive({"action": "build_barracks"})
        if response:
            return True
            #return Unit(int(response["worker_id"]))

    def get_build_location(self, building):
        pass

    def build(self, unit_type, count):
        message = {
            "action": "build",
            "unit_type": unit_type,
            "count": count
        }
        response = self.send_and_receive(message)
        if response:
            return int(response['remaining'])

    def is_started(self):
        message = {"action": "is_started"}
        try:
            response = self.send_and_receive(message)
        except RuntimeError as e:
            return False
        return bool(response)


class Unit(object):

    def __init__(self, uid):
        self.uid=uid
