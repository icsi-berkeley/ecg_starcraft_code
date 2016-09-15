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
import random
from math import sqrt
from starcraft.starcraft_game import *
from threading import Lock

import os
dir_name = os.path.dirname(os.path.realpath(__file__))

class BasicStarcraftProblemSolver(CoreProblemSolver):
    def __init__(self, args):
        CoreProblemSolver.__init__(self, args)
        self.headings = dict(north=(0.0, 1.0, 0.0), south=(0.0, -1.0, 0.0),
                    east=(1.0, 0.0, 0.0), west=(-1.0, 0.0, 0.0))

        self._recent = None
        self._wh = None

        self.world = self.build_world("world.json")

        self._units = {}

        self._inputs = []
        self._conditions = []
        self._terminate = False
        self._lock = Lock()

        self.game = Starcraft_Game(self.transport)

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
            self._inputs.append(ntuple)

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
            if len(self._inputs) > 0:
                with self._lock:
                    json_ntuple = self._inputs.pop(0)
                self.insert_condition(json_ntuple)
            if game_started:
                self.execute_conditions()
            else:
                game_started = self.game.is_started()
                if game_started:
                    print("The game has begun.")

    def insert_condition(self, json_ntuple):
        """
        Adds the described action to the conditions data structure with the right priority and
        sets it to execute the right number of times
        """
        import json # something is wrong with the json parsing. i shouldn't have to do this
        self._conditions.append(json.loads(self.decoder.convert_JSON_to_ntuple(json_ntuple)))

    def execute_conditions(self):
        """
        Executes the commands in the input data structure
        """
        completed = []
        for index, ntuple in enumerate(self._conditions):
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
            del self._conditions[index]


    def build_world(self, external_file):
        world = Struct()
        with open(os.path.join(dir_name, external_file), "r") as data:
            model = json.load(data)
        for k, v in model.items():
            value = Struct(v)
            setattr(world, k, value)
        return world


    def command_build(self, parameters):
        obj = parameters['createdThing']['objectDescriptor']
        remaining = self.game.build(obj['type'], int(obj['quantity']['amount']['value']) if obj['number'] == "plural" else 1)
        if int(remaining) > 0.0:
            if obj['number'] == "plural":
                obj['quantity']['amount']['value'] = int(remaining)
            raise RuntimeWarning("Not able to build them all")


    def solve_causal(self, parameters, predicate):
        """ This needs work. It should actually do more reasoning, to determine what precisely it's being asked to do.
        Currently, it just repackages the n-tuple and reroutes it.
        """
        agent = self.get_described_object(parameters['causalAgent']['objectDescriptor'])
        action = parameters['causalProcess']['actionary']
        parameters['actionary'], parameters['complexKind'] = action, None
        return self.route_action(parameters, predicate)


    def solve_serial(self, parameters, predicate):
        self.route_action(parameters['process1'], predicate)
        self.route_action(parameters['process2'], predicate)

    def solve_command(self, ntuple):
        parameters = ntuple['eventDescriptor']
        self.route_event(parameters, "command")

    def solve_query(self, ntuple):
        param = ntuple['eventDescriptor']
        self.route_event(param, "query")


    def get_described_process(self, objs, description):
        # TODO: Let's assume, for now, that we're just describing "one" parameterized action.
        description = description[0]
        described_action = description['actionary']
        for entry in self.history:
            parameters, successful = entry[0], entry[1]
            new_action = parameters['actionary']
            if successful and (new_action == described_action):
                dispatch = getattr(self, "match_{}".format(new_action))
                objs = dispatch(objs, parameters)
                return objs
        return objs


    def get_described_objects(self, description, multiple=False):
        if 'referent' in description:
            if hasattr(self.world, description['referent']):
                return [getattr(self.world, description['referent'])]
        obj_type = description['type']
        objs = []
        for item in self.world.__dict__.keys():
            if hasattr(getattr(self.world, item), 'type') and getattr(getattr(self.world, item), 'type') == obj_type:
                objs += [getattr(self.world, item)]
        copy = []
        if 'color' in description:
            color = description['color']
            for obj in objs:
                if obj.color == color:
                    copy.append(obj)
            objs = copy
        kind = description['kind'] if 'kind' in description else 'unmarked'
        if 'size' in description or 'weight' in description:
            size = description['size']
            objs = self.evaluate_scalar_attribute("size", size, objs, kind)
            #objs = self.evaluate_feature(size, kind, objs)
        if 'weight' in description:
            weight = description['weight']
            objs = self.evaluate_scalar_attribute("weight", weight, objs, kind)
        if 'locationDescriptor' in description:
            objs = self.get_described_location(objs, description['locationDescriptor'], multiple=multiple)

        if "relationDescriptor" in description:
            objs = self.get_described_relation(objs, description['relationDescriptor'], multiple=multiple)
        copy = []
        if "function" in description:
            for obj in objs:
                if hasattr(obj, "function") and obj.function == description['function']:
                    copy.append(obj)
            objs = copy
        if "processDescriptor" in description:
            objs = self.get_described_process(objs, description['processDescriptor'])
        # TODO: Partdescriptor
        return objs

    def filter_recent_referents(self, referent):
        types = [i.type for i in self._recent]
        if referent.type in types:
            index = types.index(referent.type)
            self._recent.remove(self._recent[index])
        self._recent.append(referent)

    def get_described_object(self, description, multiple=False):
        objs = self.get_described_objects(description, multiple)
        if len(objs) == 1:
            self.filter_recent_referents(objs[0])
            return objs[0]
        elif len(objs) > 1:
            if "givenness" in description:
                if description['givenness'] in ['typeIdentifiable', "distinct"]:
                    # Should actually iterate through _recent
                    copy = list(objs)
                    if description['givenness'] == "distinct":
                        for obj in objs:
                            if obj in self._recent:
                                copy.remove(obj)
                    # TODO: do something better than just random choice, e.g. "is a box near the blue box" (really means ANY)
                    selection = random.choice(copy)
                    self.filter_recent_referents(selection)
                    return selection
            elif self._wh:
                message = "More than one object matches the description of {}.".format(self.assemble_string(description))
                self.identification_failure(message)
                return None
            message = "Which '{}'?".format(self.assemble_string(description))
            # TODO: Tag n-tuple
            tagged = self.tag_ntuple(dict(self.ntuple), description)
            self.request_clarification(tagged, message)
            return None
        else:
            message = "Sorry, I don't know what the {} is.".format(self.assemble_string(description))
            return None

    def tag_ntuple(self, ntuple, description):
        """ Tags all ntuple keys with a "*" if the value matches DESCRIPTION. """
        new = {}
        for key, value in ntuple.items():
            if value == description:
                new["*" + key] = value
            elif type(value) == dict:
                #pass
                new[key] = self.tag_ntuple(value, description)
                # Tag ntuple on value
            else:
                new[key] = value
        return new


    def evaluate_condition(self, parameters):
        action = parameters['actionary']
        dispatch = getattr(self, "evaluate_{}".format(action))
        answer = dispatch(parameters)
        value, reason = answer['value'], answer['reason']
        if reason and not value:
            self.respond_to_query(reason)
        negated = False
        if self.p_features and 'negated' in self.p_features:
            negated = self.p_features['negated']
        if negated:
            return not value #self.evaluate_be(parameters)
        else:
            return value #self.evaluate_be(parameters)


    def compare_features(self, feature, comparator, obj1, obj2):
        f1 = getattr(obj1, feature)
        f2 = getattr(obj2, feature)
        return comparator(f1, f2)

    def evaluate_obj_predication(self, obj, predication):
        if not obj:
            return False
        kind = predication['kind'] if 'kind' in predication else 'unmarked'
        for k, v in predication.items():
            if k == "size" or k == "weight":
                ranges = self._ranges[obj.type][k]
                attribute = getattr(obj, k)
                if v > .5:
                    return attribute >= (ranges[-1]-ranges[0])/2
                else:
                    return attribute <= (ranges[-1]-ranges[0])/2
            elif k == "identical":
                return self.is_identical(obj, predication['identical']['objectDescriptor'])
            elif k == 'relation':
                if v =='near':
                    related = self.get_described_object(predication['objectDescriptor'])
                    if related and not self.is_near(obj, related):
                        return False
                    if not related:
                        return False
                if v == "in":
                    # TODO: Implement this...
                    return False
                if v == "on":
                    related = self.get_described_object(predication['objectDescriptor'])
                    if related and not self.is_on(obj, related):
                        return False
                    if not related:
                        return False
            # TODO: "Object does not have property k". Send message?
            elif hasattr(obj, k) and getattr(obj, k) != v:
                return False
        return True

    def evaluate_be(self, parameters):
        obj = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        predication = parameters['state']
        return {'value': self.evaluate_obj_predication(obj, predication), 'reason': ''}

    def is_identical(self, item, objectD):
        # Checks if it's type identifiable ("is box1 a box"), then if it's elaborated ("is box1 a red box")
        # If uniquely identifiable, just matches referred objects
        # TODO: what to return if there is more than one box?
        if (objectD['givenness'] == 'typeIdentifiable'):
            if (not 'color' in objectD) and (not 'size' in objectD):
                return item.type == objectD['type']
            else:
                return item in self.get_described_objects(objectD)
        else:
            objs = self.get_described_objects(objectD)
            if len(objs) > 1:
                return item in objs
            elif len(objs) < 1:
                return False
            else:
                return item == objs[0]


    def assertion_be(self, parameters):
        protagonist = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        state = parameters['state']
        negated = state['negated']
        if not negated:
            if 'amount' in state:
                prop, value = state['amount']['property'], state['amount']['value']
                setattr(protagonist, prop, value)
            # TODO; color, size, name?


    # Assertions not yet implemented for starcraft
    def solve_assertion(self, ntuple):
        parameters = ntuple['eventDescriptor']
        self.route_event(parameters, "assertion")
        #self.decoder.pprint_ntuple(ntuple)

    def solve_conditional_imperative(self, ntuple):
        parameters = ntuple['eventDescriptor']
        condition = parameters['condition']
        features = condition['e_features']
        if features:
            # Set eventFeatures
            self.eventFeatures = features['eventFeatures']
        core = parameters['core']
        if self.evaluate_condition(condition['eventProcess']):
            self.route_event(core, "command")
        elif parameters['else']['eventProcess']:
            self.route_event(parameters['else'], "command")
        else:
            return "Condition not satisfied."

    # Conditional declaratives not yet implemented for starcraft
    def solve_conditional_declarative(self, ntuple):
        self.decoder.pprint_ntuple(ntuple)

if __name__ == "__main__":
    solver = BasicStarcraftProblemSolver(sys.argv[1:])
