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
from nluas.Transport import Transport
import json
import time
from pprint import pprint

import os
dir_name = os.path.dirname(os.path.realpath(__file__))

class BasicStarcraftProblemSolver(CoreProblemSolver):
    def __init__(self, args):
        CoreProblemSolver.__init__(self, args)

        self.inputs = []
        self.game_started = False
        self._verbose = True

        self.adapter_address = "StarCraft"
        self.transport.subscribe(self.adapter_address, self.adapter_callback)
        self.adapter_templates = self.read_templates(
            os.path.join(dir_name, "adapter_templates.json"))

    def read_templates(self, filename):
        """ Sets each template to ordered dict."""
        base = OrderedDict()
        with open(filename, "r") as data_file:
            data = json.load(data_file, object_pairs_hook=OrderedDict)
            for name, template in data['templates'].items():
                base[name] = template
        return base

    def callback(self, actspec):
        """
        Called asychronously when an actspec is available from the specializer. Relays it to the
        Starcraft Adapter.
        """
        if self._verbose:
            print("Received ActSpec:")
            pprint(actspec)
        if self.is_quit(actspec):
            return self.close()

        self.inputs.append(actspec)
        if self.game_started:
            return self.solve()

    def adapter_callback(self, request):
        """
        Called asychronously when a request is sent from the game.
        """
        if self._verbose:
            print("Recieved adapter request")
            pprint(request)
        if "is_started" in request:
            self.game_started = True

    def adapter_command(self, command):
        """
        Sends a command to the adapter
        """
        self.validate_message(command)
        self.transport.send(self.adapter_address, json.dumps(command))

        if self._verbose:
            print("Sent to adapter:")
            pprint(command)

    def adapter_connect(self, timeout=5):
        """
        Connect to adapter before beginning sending messages
        """
        while not self.game_started:
            if self._verbose:
                print("Attempting to connect to Starcraft...")

            message = dict(self.adapter_templates["is_started"])

            send_time = time.time()
            self.adapter_command(message)

            while time.time() - send_time <= timeout:
                time.sleep(1.0 / 24.0) # Shortest length of a frame
                if self.game_started:
                    return self.solve()

    def solve(self):
        """
        Relays all pendings inputs to the correct functions to send to starcraft adapter.
        """
        completed = []
        for index, actspec in enumerate(self.inputs):
            predicate_type = actspec['predicate_type']
            try:
                dispatch = getattr(self, "solve_%s" %predicate_type)
                dispatch(actspec)
                completed.insert(0, index)
            except AttributeError as e:
                traceback.print_exc()
                message = "I cannot solve a(n) {}.".format(predicate_type)
                self.identification_failure(message)
            except RuntimeWarning as e:
                print(e)
            except RuntimeError as e:
                traceback.print_exc()
                completed.insert(0, index)
        for index in completed:
            del self.inputs[index]

    def command_build(self, parameters):
        """
        Tells the game to build an object
        """
        message = dict(self.adapter_templates["build"])
        obj = parameters['createdThing']['objectDescriptor'] #shouldn't this just be 'descriptor'?
        message["quantity"] = self.get_quantity(obj)
        message["unit_type"] = self.get_type(obj)

        message["ecg_id"] = None # TODO

        self.adapter_command(message)

    def command_gather(self, parameters):
        """
        Tells the game to build an object
        """
        message = dict(self.adapter_templates["gather"])
        obj = parameters['resource']['objectDescriptor'] #shouldn't this just be 'descriptor'?
        message["resource_type"] = "MINERALS" if self.get_type(obj) == 'mineral' else "GAS"

        self.adapter_command(message)

    def command_move(self, parameters):
        """
        Tells the game to move a unit
        """
        message = dict(self.adapter_templates["move"])
        message["location"] = dict(self.adapter_templates["location_descriptor"])
        if parameters['spg']['spgDescriptor']['goal']:
            pass #TODO
        elif parameters['heading']:
            message["location"]["region"] = "EXACT"
            message["location"]["landmark"] = None
        else:
            raise RuntimeError("Invalid ActSpec")
        message["commanded_unit"] = None #TODO

        self.adapter_command(message)

    def command_squad(self, parameters):
        raise NotImplementedError()

    def command_defend(self, parameters):
        raise NotImplementedError()

    def command_attack(self, parameters):
        raise NotImplementedError()

    def get_quantity(self, objectDescriptor):
        if objectDescriptor['number'] == "plural":
            return int(objectDescriptor['quantity']['amount']['value'])
        return 1

    def get_type(self, objectDescriptor):
        return objectDescriptor['type']

    def solve_serial(self, parameters, predicate):
        """
        Solves a serial event
        """
        self.route_action(parameters['process1'], predicate)
        self.route_action(parameters['process2'], predicate)

    def solve_command(self, actspec):
        """
        Solves a command
        """
        parameters = actspec['eventDescriptor']
        self.route_event(parameters, "command")

    def validate_message(self, message):
        template = self.adapter_templates[message["type"]]
        if "parents" in template:
            for parent_type in template["parents"]:
                parent = self.adapter_templates[parent_type]
                for k, v in parent.items():
                    if k not in template:
                        template[k] = v

        for k, v in template.items():
            if v[0] == "*":
                if v == None:
                    raise RuntimeError("Missing required field %s. Invalid message: %s" %(k, message))
                v = v[1:]

            if k == "type" or k == "parents":
                pass
            elif v == "INTEGER" and not (isinstance(message[k], int) or message[k] == None):
                raise RuntimeError("Incorrect integer for %s. Invalid message: %s" %(k, message))
            elif v == "STRING" and not (isinstance(message[k], str) or message[k] == None):
                raise RuntimeError("Incorrect string for %s. Invalid message: %s" %(k, message))
            elif v == "BOOLEAN" and not (isinstance(message[k], bool) or message[k] == None):
                raise RuntimeError("Incorrect boolean for %s. Invalid message: %s" %(k, message))
            elif v.startswith("MC"):
                options = v.split(":")[1].split("|")
                if message[k] not in options:
                    raise RuntimeError("Incorrect MC for %s. Invalid message: %s" %(k, message))
            elif v.startswith("OBJECT"):
                if message[k] == None:
                    continue
                valid_types = v.split(":")[1].split("|")
                object_types = [message[k]["type"], ]
                if "parents" in message[k]:
                    object_types += message[k]["parents"]

                matches = [typ for typ in object_types if typ in valid_types]
                if len(matches) == 0:
                    raise RuntimeError("Incorrect object type for %s. Invalid message: %s" %(k, message))

                self.validate_message(message[k])
            else:
                pass


    def temporary_hack(self):
        while self._keep_alive:
            choice = input(">")
            message = None
            if choice == "":
                continue
            elif choice == "q":
                return exit()
            elif choice == "2" or choice == "4":
                # "Mine the minerals!"
                message = {
                    "type": "gather",
                    "parents": ["action"],
                    "resource_type": "MINERALS",
                    "commanded_unit": {
                        "type": "unit_descriptor",
                        "quantity": 0,
                        "comparator": "GEQ",
                        "name": None,
                        "unit_type": None, #NOTE: This used to be "scv" which was too specific
                        "ecg_id": None,
                        "location": None,
                        "status": "NA",
                        "ally": True
                    }
                }
            elif choice == "3":
                # "Harvest the gas!"
                message = {
                    "type": "gather",
                    "parents": ["action"],
                    "resource_type": "GAS",
                    "commanded_unit": {
                        "type": "unit_descriptor",
                        "quantity": 0,
                        "comparator": "GEQ",
                        "name": None,
                        "unit_type": None,
                        "ecg_id": None,
                        "location": None,
                        "status": "NA",
                        "ally": True
                    }
                }
            elif choice == "5":
                # "Delta, mine the minerals!"
                message = {
                    "type": "gather",
                    "parents": ["action"],
                    "resource_type": "MINERALS",
                    "commanded_unit": {
                        "type": "unit_descriptor",
                        "quantity": 0,
                        "comparator": "GEQ",
                        "name": "delta",
                        "unit_type": None,
                        "ecg_id": None,
                        "location": None,
                        "status": "NA",
                        "ally": True
                    }
                }
            elif choice == "6":
                # “Build an SCV!”
                message = {
                    "type": "build",
                    "parents": ["action"],
                    "quantity": 1,
                    "unit_type": "scv",
                    "ecg_id": 123456789
                }
            elif choice == "7":
                # “Build 5 marines!”
                message = {
                    "type": "build",
                    "parents": ["action"],
                    "quantity": 5,
                    "unit_type": "marine",
                    "ecg_id": 123456789
                }
            elif choice == "8":
                # “Build a barracks!”
                message = {
                    "type": "build",
                    "parents": ["action"],
                    "quantity": 1,
                    "unit_type": "barracks",
                    "ecg_id": 123456789
                }
            elif choice == "26":
                # “If an SCV is idle, make it mine minerals!”
                message = {
                    "type": "conditional",
                    "trigger": "ALWAYS",
                    "event": {
                        "type": "army",
                        "parents": ["event"],
                        "unit_descriptor": {
                            "type": "unit_descriptor",
                            "quantity": 1,
                            "comparator": "GEQ",
                            "name": None,
                            "unit_type": "scv",
                            "ecg_id": None,
                            "location": None,
                            "status": "IDLE",
                            "ally": True
                        }
                    },
                    "response": {
                        "type": "gather",
                        "parents": ["action"],
                        "resource_type": "MINERALS",
                        "commanded_unit": {
                            "type": "unit_descriptor",
                            "quantity": 1,
                            "comparator": "GEQ",
                            "name": None,
                            "unit_type": "scv",
                            "ecg_id": None,
                            "location": None,
                            "status": "IDLE",
                            "ally": True
                        }
                    }
                }
            elif choice == "27":
                # “Build SCVs until I have 8 of them!”
                message = {
                    "type": "conditional",
                    "trigger": "UNTIL",
                    "event": {
                        "type": "army",
                        "parents": ["event"],
                        "unit_descriptor": {
                            "type": "unit_descriptor",
                            "quantity": 8,
                            "comparator": "GEQ",
                            "name": None,
                            "unit_type": "scv",
                            "ecg_id": None,
                            "location": None,
                            "status": "NA",
                            "ally": True
                        }
                    },
                    "response": {
                        "type": "build",
                        "parents": ["action"],
                        "quantity": 1,
                        "unit_type": "scv",
                        "ecg_id": 0
                    }
                }
            else:
                print("only choices 2, 3 and 4 are currently supported.")
                pass

            if message:
                self.adapter_command(message)


if __name__ == "__main__":
    solver = BasicStarcraftProblemSolver(sys.argv[1:])
    solver.adapter_connect()
    solver.temporary_hack()
