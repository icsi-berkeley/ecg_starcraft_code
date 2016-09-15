"""

Author: vivekraghuram <vivek.raghuram@berkeley.edu>

A StarcraftProblemSolver that extends the CoreProblemSolver in the NLUAS module.

Actions, like "move", should be named by predicate + action type.
Thus: query_move, command_move, etc.
Or: query_be, command_be, etc.

"""

from nluas.app.core_solver import *
from nluas.utils import *
import sys
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

class BasicStarcraftProblemSolver(CoreProblemSolver):
    def __init__(self, args):
        CoreProblemSolver.__init__(self, args)
        self.headings = dict(north=(0.0, 1.0, 0.0), south=(0.0, -1.0, 0.0),
                    east=(1.0, 0.0, 0.0), west=(-1.0, 0.0, 0.0))

        self._recent = None
        self._wh = None

        self._units = {}

        self._inputs = []
        self._terminate = False
        self._lock = Lock()
        self.adapter_address = "StarCraft"
        self._response = None
        self.transport.subscribe(self.adapter_address, self.adapter_callback)

        self.solve()

    @run_async
    def callback(self, ntuple, *args, **kwargs):
        """
        Called asychronously when a ntuple is available from the specializer. Adds it to a
        queue of inputs.
        """
        if ntuple == '"QUIT"':
            return self.close()
        with self._lock:
            self._inputs.append(json.loads(self.decoder.convert_JSON_to_ntuple(ntuple)))

    @run_async
    def adapter_callback(self, response, *args, **kwargs):
        """
        Called asychronously when a response is sent from the game. Adds it to a
        queue of game responses.
        """
        with self._lock:
            self._response = json.loads(response)

    @run_async
    def close(self):
        """
        Called to terminate the solver's while loop.
        """
        with self._lock:
            self._terminate = True

    def solve(self):
        """
        Checks the input queue and inserts any pending inputs to the conditions data structure.
        Executes conditions once all inputs have been processed.
        """
        game_started = False
        while not self._terminate:
            if game_started:
                self.execute_commands()
            else:
                game_started = self.is_started()
                if game_started:
                    print("The game has begun.")

    def execute_commands(self):
        """
        Executes the commands in the input data structure
        """
        completed = []
        for index, ntuple in enumerate(self._inputs):
            predicate_type = ntuple['predicate_type']
            try:
                dispatch = getattr(self, "solve_%s" %predicate_type)
                dispatch(ntuple)
                completed.insert(0, index)
            except AttributeError as e:
                traceback.print_exc()
                message = "I cannot solve a(n) {}.".format(predicate_type)
                self.identification_failure(message)
            except RuntimeWarning as e:
                pass
            except RuntimeError as e:
                print(e)
        for index in completed:
            del self._inputs[index]

    def send_and_receive(self, message, timeout=5):
        """
        Sends message and waits for a response with response_head
        """
        send_time = time.time()
        self._response = None
        response_head = str(send_time) # DELETE ME
        message["response_head"] = response_head
        self.transport.send(self.adapter_address, json.dumps(message))
        print("sent: ", message)

        while time.time() - send_time <= timeout:
            if self._response:
                print("received: ", self._response)
                if self._response["status"] == "success" or 'remaining' in self._response:
                    return self._response
                raise RuntimeWarning("Could not complete: " + str(message))
        raise RuntimeError("Command timed out: " + str(message))


    def is_started(self):
        message = {"action": "is_started"}
        try:
            response = self.send_and_receive(message)
        except RuntimeError as e:
            return False
        return bool(response)

    def command_build(self, parameters):
        obj = parameters['createdThing']['objectDescriptor']
        message = {
            "action": "build",
            "unit_type": obj['type'],
            "count": int(obj['quantity']['amount']['value']) if obj['number'] == "plural" else 1
        }
        response = self.send_and_receive(message)
        if "remaining" in response and int(response["remaining"]) > 0.0:
                if obj['number'] == "plural":
                    obj['quantity']['amount']['value'] = int(response["remaining"])
                raise RuntimeWarning("Not able to build them all")


    def solve_serial(self, parameters, predicate):
        self.route_action(parameters['process1'], predicate)
        self.route_action(parameters['process2'], predicate)

    def solve_command(self, ntuple):
        parameters = ntuple['eventDescriptor']
        self.route_event(parameters, "command")


if __name__ == "__main__":
    solver = BasicStarcraftProblemSolver(sys.argv[1:])
