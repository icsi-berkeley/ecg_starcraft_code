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
        self._next_ecg_id = 1234
        self._current_ecg_id = None
        self._quantity_modifier = 0

        self.adapter_address = "StarCraft"
        self.transport.subscribe(self.adapter_address, self.adapter_callback)
        self.text_address = "{}_{}".format(self.federation, "TextAgent")
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
        if "command" in request:
            self.transport.send(self.text_address, json.dumps(request))

    def adapter_command(self, command):
        """
        Sends a command to the adapter
        """
        self.validate_message(command)
        self.transport.send(self.adapter_address, json.dumps(command))
        self._current_ecg_id = None

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
                message = dispatch(actspec)
                self.adapter_command(message)
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
        message["ecg_id"] = self.get_ecg_id(True)
        return message

    def command_gather(self, parameters):
        """
        Tells the game to build an object
        """
        message = dict(self.adapter_templates["gather"])
        resource = parameters['resource']['objectDescriptor'] #shouldn't this just be 'descriptor'?
        message["resource_type"] = "MINERALS" if self.get_type(resource) == 'mineral' else "GAS"
        message["commanded_unit"] = self.get_unit_descriptor(parameters['protagonist']['objectDescriptor'])
        return message

    def command_move(self, parameters):
        """
        Tells the game to move a unit
        """
        message = dict(self.adapter_templates["move"])
        if parameters['spg']['spgDescriptor']['goal']:
            message["location"] = self.get_location_descriptor(parameters['spg']['spgDescriptor']['goal']['locationDescriptor'])
        elif parameters['heading']:
            message["location"] = dict(self.adapter_templates["location_descriptor"])
            message["location"]["region"] = "EXACT"
            message["location"]["landmark"] = None
        else:
            raise RuntimeError("Invalid ActSpec")
        message["commanded_unit"] = self.get_unit_descriptor(parameters['protagonist']['objectDescriptor'])
        return message

    def command_squad(self, parameters):
        raise NotImplementedError()

    def command_defend(self, parameters):
        raise NotImplementedError()

    def command_attack(self, parameters):
        raise NotImplementedError()

    def command_communication(self, parameters):
        # need to do something in order to pass the listener on as the protagonist of the content
        content = parameters['content']
        content['eventProcess']['protagonist'] = parameters['listener']
        return self.route_event(content, 'command')

    def assertion_possess(self, parameters):
        if parameters['possessed']['objectDescriptor']['type'] not in ['mineral', 'gas', 'supply']:
            message = dict(self.adapter_templates["army"])
            message['unit_descriptor'] = self.get_unit_descriptor(parameters['possessed']['objectDescriptor'])
            return message
        raise Exception("Not implemented assertion") #TODO support more assertions

    def assertion_be(self, parameters):
        message = dict(self.adapter_templates["resource"])
        if parameters['state']['relation'] == 'at':
            message["comparator"] = "EQ"
            if parameters['state']['objectDescriptor']['type'] == 'limit':
                message["threshold"] = -1
                if parameters['state']['objectDescriptor']['modifier']['objectDescriptor']['type'] == 'supply2':
                    message["resource_type"] = "SUPPLY"
                    return message
        raise Exception("Not implemented assertion")

    def get_location_descriptor(self, locationDescriptor):
        location_descriptor = dict(self.adapter_templates["location_descriptor"])
        location_descriptor["region"] = self.get_region(locationDescriptor)
        location_descriptor["landmark"] = self.get_unit_descriptor(locationDescriptor['objectDescriptor'])
        return location_descriptor

    def get_region(self, locationDescriptor):
        # TODO: Add more regions
        if locationDescriptor['relation'] == 'behind':
            return "BACK"
        return "EXACT"

    def get_unit_descriptor(self, objectDescriptor):
        unit_descriptor = dict(self.adapter_templates["unit_descriptor"])
        unit_descriptor["name"] = self.get_name(objectDescriptor)
        unit_descriptor["ecg_id"] = self.get_ecg_id()
        unit_descriptor["status"] = self.get_status(objectDescriptor)
        unit_descriptor["comparator"] = self.get_direction(objectDescriptor)
        unit_descriptor["quantity"] = self.get_quantity(objectDescriptor, True)
        unit_descriptor["ally"] = self.get_ally(objectDescriptor)
        unit_descriptor["unit_type"] = self.get_type(objectDescriptor)
        unit_descriptor["location"] = None # FIXME
        return unit_descriptor

    def get_direction(self, objectDescriptor):
        if 'direction' in objectDescriptor:
            if objectDescriptor['direction'] == 'down':
                self._quantity_modifier = -1
                return "LEQ"
            elif objectDescriptor['direction'] == 'up':
                self._quantity_modifier = 1
                return "GEQ"
        elif 'quantity' in objectDescriptor and 'amount' in objectDescriptor['quantity']:
            return "EQ"
        return "GEQ"

    def get_quantity(self, objectDescriptor, use_quantity_modifier = False):
        if 'quantity' in objectDescriptor and 'amount' in objectDescriptor['quantity']:
            if use_quantity_modifier:
                return int(objectDescriptor['quantity']['amount']['value']) + self._quantity_modifier
            return int(objectDescriptor['quantity']['amount']['value'])
        return 1

    def get_type(self, objectDescriptor):
        if 'type' in objectDescriptor and objectDescriptor['type'] not in ['robot', 'sentient']:
            return objectDescriptor['type']
        return None

    def get_status(self, objectDescriptor):
        # TODO: Add more statuses
        if 'status' in objectDescriptor:
            if objectDescriptor['status'] == 'idle':
                return "IDLE"
        return "NA"

    def get_name(self, objectDescriptor):
        if 'referent' in objectDescriptor and objectDescriptor['referent'] not in ['sentient']:
            return objectDescriptor['referent']
        return None

    def get_ally(self, objectDescriptor):
        if 'alliance' in objectDescriptor:
            return objectDescriptor['alliance'] == 'ally'
        return True

    def get_ecg_id(self, new_id = False):
        if new_id:
            self._current_ecg_id = self._next_ecg_id
            self._next_ecg_id += 1
        return self._current_ecg_id

    def route_event(self, eventDescription, predicate):
        if "complexKind" in eventDescription and eventDescription['complexKind'] == "serial":
            return self.solve_serial(eventDescription, predicate)
        if "complexKind" in eventDescription and eventDescription['complexKind'] == "conditional":
            dispatch = getattr(self, "solve_conditional_{}".format(predicate))
            return dispatch(eventDescription)
        features = eventDescription['e_features']
        if features:
            # Set eventFeatures
            self.eventFeatures = features['eventFeatures']
        parameters = eventDescription['eventProcess']
        return self.route_action(parameters, predicate)

    def solve_serial(self, parameters, predicate):
        """
        Solves a serial event
        """
        message = dict(self.adapter_templates["sequential"])
        message['first'] = self.route_event(parameters['condition'], predicate)
        message['second'] = self.route_event(parameters['core'], predicate)
        return message

    def solve_command(self, actspec):
        """
        Solves a command
        """
        parameters = actspec['eventDescriptor']
        return self.route_event(parameters, "command")

    def solve_conditional_command(self, actspec):
        """ Takes in conditionalED."""
        message = dict(self.adapter_templates["conditional"])
        message['event'] = self.route_event(actspec['condition'], 'assertion')
        message['response'] = self.route_event(actspec['core'], 'command')
        message['trigger'] = "ALWAYS" # TODO: generalize for other triggers
        return message

    def solve_causal(self, parameters, predicate):
        action = parameters['affectedProcess']['actionary']
        dispatch = getattr(self, "{}_{}".format(predicate, action))
        return dispatch(parameters['affectedProcess'])

    def validate_message(self, message):
        print(message)
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
                        "unit_type": None,
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
                    "ecg_id": 123
                }
            elif choice == "7":
                # “Build 5 marines!”
                message = {
                    "type": "build",
                    "parents": ["action"],
                    "quantity": 5,
                    "unit_type": "marine",
                    "ecg_id": 1234
                }
            elif choice == "8":
                # “Build a barracks!”
                message = {
                    "type": "build",
                    "parents": ["action"],
                    "quantity": 1,
                    "unit_type": "barracks",
                    "ecg_id": 12345
                }
            elif choice == "11":
                # "Delta, move over here!"
                message = {
                    "type": "move",
                    "parents": ["action"],
                    "location": {
                        "type": "location_descriptor",
                        "landmark": None,
                        "region": "EXACT"
                    },
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
            elif choice == "12":
                # Delta, move behind the barracks!
                message = {
                    "type": "move",
                    "parents": ["action"],
                    "location": {
                        "type": "location_descriptor",
                        "landmark": {
                            "type": "unit_descriptor",
                            "quantity": 1,
                            "comparator": "GEQ",
                            "name": None,
                            "unit_type": "barracks",
                            "ecg_id": None,
                            "location": None,
                            "status": "NA",
                            "ally": True
                        },
                        "region": "BACK"
                    },
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
            elif choice == "13":
                # Move behind the barracks!
                message = {
                    "type": "move",
                    "parents": ["action"],
                    "location": {
                        "type": "location_descriptor",
                        "landmark": {
                            "type": "unit_descriptor",
                            "quantity": 1,
                            "comparator": "GEQ",
                            "name": None,
                            "unit_type": "barracks",
                            "ecg_id": None,
                            "location": None,
                            "status": "NA",
                            "ally": True
                        },
                        "region": "BACK"
                    },
                    "commanded_unit": None
                }
            elif choice == "14":
                # Move here!
                message = {
                    "type": "move",
                    "parents": ["action"],
                    "location": {
                        "type": "location_descriptor",
                        "landmark": None,
                        "region": "EXACT"
                    },
                    "commanded_unit": None
                }
            elif choice == "15":
                # Move the 5 closest marines here!
                message = {
                    "type": "move",
                    "parents": ["action"],
                    "location": {
                        "type": "location_descriptor",
                        "landmark": None,
                        "region": "EXACT"
                    },
                    "commanded_unit": {
                        "type": "unit_descriptor",
                        "quantity": 5,
                        "comparator": "EQ",
                        "name": None,
                        "unit_type": "marine",
                        "ecg_id": None,
                        "location": {
                            "type": "location_descriptor",
                            "landmark": None,
                            "region": "CLOSE"
                        },
                        "status": "NA",
                        "ally": True
                    }
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
                        "ecg_id": 123456
                    }
                }
            elif choice == "28":
                #Whenever I reach the supply limit, build a supply depot!
                message = {
                    "type": "conditional",
                    "trigger": "ALWAYS",
                    "event": {
                        "type": "resource",
                        "parents": ["event"],
                        "resource_type": "SUPPLY",
                        "threshold": -1,
                        "comparator": "EQ"
                    },
                    "response": {
                        "type": "build",
                        "parents": ["action"],
                        "quantity": 1,
                        "unit_type": "supplydepot",
                        "ecg_id": 234
                    }
                }
            elif choice == "29":
                #While I have more than 100 minerals, build marines!
                message = {
                    "type": "conditional",
                    "trigger": "WHILE",
                    "event": {
                        "type": "resource",
                        "parents": ["event"],
                        "resource_type": "MINERALS",
                        "threshold": 101,
                        "comparator": "GEQ"
                    },
                    "response": {
                        "type": "build",
                        "parents": ["action"],
                        "quantity": 1,
                        "unit_type": "marine",
                        "ecg_id": 2345
                    }
                }
            elif choice == "32":
                #Build a barracks and then build 5 SCVs!
                message = {
                  "type": "sequential",
                  "first": {
                      "type": "build",
                      "parents": ["action"],
                      "quantity": 1,
                      "unit_type": "barracks",
                      "ecg_id": 23456
                  },
                  "second": {
                      "type": "build",
                      "parents": ["action"],
                      "quantity": 5,
                      "unit_type": "scv",
                      "ecg_id": 234567
                  }
                }
            elif choice == "33":
                #Build 2 SCVs and then move them here!
                message = message = {
                  "type": "sequential",
                  "first": {
                      "type": "build",
                      "parents": ["action"],
                      "quantity": 2,
                      "unit_type": "scv",
                      "ecg_id": 345
                  },
                  "second": {
                      "type": "move",
                      "parents": ["action"],
                      "location": {
                          "type": "location_descriptor",
                          "landmark": None,
                          "region": "EXACT"
                      },
                      "commanded_unit": {
                          "type": "unit_descriptor",
                          "quantity": 2,
                          "comparator": "GEQ",
                          "name": None,
                          "unit_type": "scv",
                          "ecg_id": 345,
                          "location": None,
                          "status": "NA",
                          "ally": True
                      }
                  }
                }
            elif choice == "36":
                #If I ever have less than 10 SCVs, build an SCV and make it mine minerals!
                message = {
                    "type": "conditional",
                    "trigger": "ALWAYS",
                    "event": {
                        "type": "army",
                        "parents": ["event"],
                        "unit_descriptor": {
                            "type": "unit_descriptor",
                            "quantity": 9,
                            "comparator": "LEQ",
                            "name": None,
                            "unit_type": "scv",
                            "ecg_id": None,
                            "location": None,
                            "status": "NA",
                            "ally": True
                        }
                    },
                    "response": {
                        "type": "sequential",
                        "first": {
                            "type": "build",
                            "parents": ["action"],
                            "quantity": 1,
                            "unit_type": "scv",
                            "ecg_id": 3456
                        },
                        "second": {
                            "type": "gather",
                            "parents": ["action"],
                            "resource_type": "MINERALS",
                            "commanded_unit": {
                                "type": "unit_descriptor",
                                "quantity": 1,
                                "comparator": "GEQ",
                                "name": None,
                                "unit_type": "scv",
                                "ecg_id": 3456,
                                "location": None,
                                "status": "NA",
                                "ally": True
                            }
                        }
                    }
                }
            # elif choice == "37":
            #     #If I have fewer than 3 SCVs harvesting gas, build an SCV and make it harvest gas!
            #     message =
            else:
                print("only choices 2, 3 and 4 are currently supported.")
                pass

            if message:
                self.adapter_command(message)


if __name__ == "__main__":
    solver = BasicStarcraftProblemSolver(sys.argv[1:])
    solver.adapter_connect()
    # solver.temporary_hack()
