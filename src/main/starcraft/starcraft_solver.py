"""

Author: seantrott <seantrott@icsi.berkeley.edu>

A StarcraftProblemSolver that extends the CoreProblemSolver in the NLUAS module.

Actions, like "move", should be named by predicate + action type.
Thus: query_move, command_move, etc.
Or: query_be, command_be, etc.

Questions:
(1) Keep in mind: should we route based on actions? E.g., "move to the blue box?"
This was how it was done previously, but what about queries/assertions: "he moved to the blue box",
"can you move to the blue box?", "which box did you move?"

Possible solutions:
(1) Separate "information" functions like gathering information from an n-tuple and matching to world,
from action/query functions. Thus, we'd have several functions involving "move", including:
-"do_move"
-"move_info": gather information (used in do_move)

"""

from nluas.app.core_solver import *
from nluas.utils import *
import sys
import random
from math import sqrt

import os
dir_name = os.path.dirname(os.path.realpath(__file__))

class BasicStarcraftProblemSolver(CoreProblemSolver):
    def __init__(self, args):
        CoreProblemSolver.__init__(self, args)
        self.headings = dict(north=(0.0, 1.0, 0.0), south=(0.0, -1.0, 0.0),
                    east=(1.0, 0.0, 0.0), west=(-1.0, 0.0, 0.0))

        self._recent = None
        self._wh = None

        self._units = {}

        import subprocess
        subprocess.Popen(["say", "setting up problem solver"])

    def build_world(self, external_file):
        world = Struct()
        with open(os.path.join(dir_name, external_file), "r") as data:
            model = json.load(data)
        for k, v in model.items():
            value = Struct(v)
            setattr(world, k, value)
        return world

    def euclidean_distance(self, p, q):
        """ Gets euclidean distance between two points. Takes in points, not objects. Expects points in dictionary format."""
        return sqrt(pow((p['x']-q['x'] ),2) + pow((p['y']-q['y'] ),2) )


    def solve_causal(self, parameters, predicate):
        """ This needs work. It should actually do more reasoning, to determine what precisely it's being asked to do.
        Currently, it just repackages the n-tuple and reroutes it.
        """
        agent = self.get_described_object(parameters['causalAgent']['objectDescriptor'])
        action = parameters['causalProcess']['actionary']
        parameters['actionary'], parameters['complexKind'] = action, None
        return self.route_action(parameters, predicate)


    def set_home(self, ntuple):
        parameters = ntuple['eventDescriptor']['eventProcess']
        prot = parameters['protagonist']
        obj = self.get_described_object(prot['objectDescriptor'])
        if obj:
            self._home = obj.pos

    def solve_serial(self, parameters, predicate):
        self.route_action(parameters['process1'], predicate)
        self.route_action(parameters['process2'], predicate)

    def solve_command(self, ntuple):
        self.set_home(ntuple)
        parameters = ntuple['eventDescriptor']
        self.route_event(parameters, "command")

    def solve_query(self, ntuple):
        param = ntuple['eventDescriptor']
        self.route_event(param, "query")


    def evaluate_can_bring(self, parameters):
        """ TODO: determine whether robot can pick up object."""
        info = self.get_bring_info(parameters)

        return {'value': True, 'reason': 'it is possible'}

    def command_pickup(self, parameters):
        info = self.get_pickup_info(parameters)
        if not info['actedUpon']:
            return None
        answer = self.evaluate_can_grasp(info)
        if answer['value']:
            self.pickup(info)

        else:
            return answer['reason']

    def command_grasp(self, parameters):
        return self.command_pickup(parameters)

    def query_pickup(self, parameters):
        #info = self.get_pickup_info(parameters)
        answer = self.evaluate_pickup(parameters)
        value, reason = answer['value'], answer['reason']
        msg = "Yes." if value else "No, {}".format(reason)
        return msg



    def evaluate_pickup(self, parameters):
        if self.eventFeatures and "modality" in self.eventFeatures and self.eventFeatures['modality'] == "can":
            negated = self.eventFeatures['negated']
            # TODO: what to do about negated, here?
            info = self.get_pickup_info(parameters)
            return self.evaluate_can_grasp(info)


    def evaluate_can_grasp(self, info):
        """ Check amount of work required to pick up object. Check if object is graspable (TODO). """
        #info = self.get_pickup_info(parameters)
        if not info['actedUpon']:
            return {'value': False, 'reason': "I don't know what you want me to pick up."}
        work = self.calculate_lift_work(info['actedUpon'].weight * 9.8, 1)
        # Also check if object is 'graspable'
        if work > info['protagonist'].fuel:
            return {'value': False, 'reason': 'not enough fuel'}
        else:
            return {'value': True, 'reason': 'it is possible'}


    def get_pickup_info(self, parameters):
        actedUpon = self.get_described_object(parameters['manipulated_entity']['objectDescriptor'])
        protagonist = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        return dict(actedUpon=actedUpon, protagonist=protagonist)

    def command_bring(self, parameters):
        """ This will move to the object location, grasp the object, and then move to another location. """
        information = self.get_bring_info(parameters)
        answer = self.evaluate_can_push_move(parameters)
        actor = information['protagonist']
        if not information['actedUpon']:
            return None
        answer = self.evaluate_can_grasp(information)
        if answer and answer['value']:
            if information['goal']:
                final_destination = information['goal']
            elif information['heading']:
                temp = self.get_push_direction_info(information['heading']['headingDescriptor'], information['actedUpon'], information['distance']['scaleDescriptor']['value'])
                final_destination = dict(x=temp['x2'], y=temp['y2'])
            self.bring(actor, information, final_destination, information['goal_object'])

        else:
            return answer['reason']

    def get_bring_info(self, parameters):
        """ Same body as get_push_info, since they use similar structures. """
        heading = parameters['affectedProcess']['heading']['headingDescriptor']
        protagonist = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        goal = parameters['affectedProcess']['spg']['spgDescriptor']['goal']


        distance = parameters['affectedProcess']['distance']
        info = dict(goal=None,
                    heading=dict(headingDescriptor=None),
                    actedUpon=None,
                    distance=None,
                    protagonist=None)
        obj = self.get_described_object(parameters['affectedProcess']['protagonist']['objectDescriptor'])
        info['actedUpon'] = obj
        if goal:
            info['goal'] = self.goal_info(goal)
        info['goal_object'] = goal # HACK for bring for ROS and Manfred
        info['heading']['headingDescriptor'] = parameters['affectedProcess']['heading']['headingDescriptor'] #self.heading_info(obj, parameters.affectedProcess['heading'], distance)
        info['distance'] = distance
        info['protagonist'] = protagonist
        return info


    def command_move(self, parameters):
        information = self.get_move_info(parameters)
        destination = information['destination']
        if destination:
            self.move(information['protagonist'], destination['x'], destination['y'], destination['z'],
                information['speed'], tolerance=3.5)
        else:
            pass
            # TO DO: What to do here?
            #print("Command_move, no destination.")

    def get_move_info(self, parameters):
        information = dict(destination=None,
                           protagonist=None,
                           speed=None)

        information['protagonist'] = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        information['speed'] = parameters['speed'] * self._speed
        #spg = selparameters['spg']

        spg = parameters['spg']['spgDescriptor']

        if spg['goal']:
            information['destination'] =self.goal_info(spg['goal'], information['protagonist'])
        elif parameters['heading']['headingDescriptor']:
            information['destination'] = self.heading_info(information['protagonist'], parameters['heading']['headingDescriptor'], parameters['distance']['scaleDescriptor'])
        return information



    def goal_info(self, goal, protagonist=None):
        destination = dict(x=None, y=None, z=0.0)
        if "location" in goal:
            if goal['location'] == 'home':
                # Determine "home" position
                destination['x'] = self._home.x
                destination['y'] = self._home.y
            else:
                destination['x'] = goal['location'][0]
                destination['y'] = goal['location'][1]
                # Z point?
                #destination['z'] = goal['location'][0]
        elif 'objectDescriptor' in goal:
            obj = self.get_described_object(goal['objectDescriptor'], multiple=True)
            #print(obj)
            if obj:
                destination['x'] = obj.pos['x']
                destination['y'] = obj.pos['y']
                destination['z'] = obj.pos['z']
            else:
                return None
        elif "locationDescriptor" in goal:
            properties = goal['locationDescriptor']
            position = self.get_described_position(properties, protagonist)
            destination['x'], destination['y'], destination['z'] = position[0], position[1], position[2]
        return destination

    def getpos(self, inst):
        p = getattr(getattr(self.world, inst), 'pos')
        return (p['x'], p['y'], p['z'])

    def heading_info(self, protagonist, heading, distance):
        n = float(distance['value'])
        # units?
        #name = getattr(protagonist, 'name')
        #pos = self.getpos(name)
        pos = (protagonist.pos['x'], protagonist.pos['y'], protagonist.pos['z'])
        newpos = vector_add(pos, vector_mul(n, self.headings[heading]))
        return dict(x=newpos[0], y=newpos[1], z=newpos[2])



    def command_push_move(self, parameters):
        #protagonist = self.get_described_object(parameters.causer['objectDescriptor'])
        info = self.get_push_info(parameters)
        if not info['actedUpon']:
            return None
        if info['goal']:
            # Create self.push_to_location
            answer = self.evaluate_can_push_move(parameters)
            self.push_to_location(info['actedUpon'], info['goal'], info['protagonist'])

        elif info['heading']:
            answer = self.evaluate_can_push_move(parameters)
            if answer['value']:
                self.push_direction(info['heading']['headingDescriptor'], info['actedUpon'], info['distance'], info['protagonist'])
            else:
                return answer['reason']

    def push_to_location(self, actedUpon, goal, protagonist):
        self.identification_failure(message=self._incapable)
        og = actedUpon.pos
        #print("Original location: {}".format(og))
        #print("Goal location: {}".format(goal))


    def push_direction(self, heading, actedUpon, distance, protagonist):
        info = self.get_push_direction_info(heading, actedUpon, distance['scaleDescriptor']['value'])
        self.move(protagonist, info['x1'], info['y1'], tolerance=3)
        self.move(protagonist, info['x2'], info['y2'], tolerance=3, collide=True)


    def get_push_direction_info(self, heading, obj, distance):
        addpos = vector_mul(-6, self.headings[heading])
        addpos2 = vector_mul(distance, self.headings[heading])
        return {'x1': obj.pos['x'] + addpos[0],
                'y1': obj.pos['y'] + addpos[1],
                'x2': obj.pos['x'] + addpos2[0],
                'y2': obj.pos['y'] + addpos2[1]}



    def get_push_info(self, parameters):
        heading = parameters['affectedProcess']['heading']['headingDescriptor']
        protagonist = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        goal = parameters['affectedProcess']['spg']['spgDescriptor']['goal']
        distance = parameters['affectedProcess']['distance']
        info = dict(goal=None,
                    heading=dict(headingDescriptor=None),
                    actedUpon=None,
                    distance=None,
                    protagonist=None)
        obj = self.get_described_object(parameters['affectedProcess']['protagonist']['objectDescriptor'])
        info['actedUpon'] = obj
        if goal:
            info['goal'] = self.goal_info(goal)
        info['heading']['headingDescriptor'] = parameters['affectedProcess']['heading']['headingDescriptor'] #self.heading_info(obj, parameters.affectedProcess['heading'], distance)
        info['distance'] = distance
        info['protagonist'] = protagonist
        return info



    def distance(self, a, b):
        return sqrt(pow((a.pos['x']-b.pos['x'] ),2) + pow((a.pos['y']-b.pos['y'] ),2) )

    def get_near(self, candidates, obj):
        locations = []
        for candidate in candidates:
            if self.is_near(candidate, obj):
                locations.append(candidate)
        return locations

    def get_threshold(self, first, second):
        multiplier = (self._distance_multipliers[first.type] * first.size) + (self._distance_multipliers[second.type] * second.size)
        return self._distance_threshold + multiplier

    def is_near(self, first, second):
        """ Could be redone. Essentially an arbitrary threshold. Could be rewritten
        to evaluate "near" in a more relativistic way.
        Could also take size into account. """
        if first == second:
            return False
        #t = self.get_threshold(first, second)
        #print(t)
        return self.distance(first, second) <= self.get_threshold(first, second)


    def is_on(self, first, second):
        """ Could be redone. Just tests whether the z value of second is higher than first, and x and y are close."""

        print(first)
        print(second)
        if first == second:
            return False
        #t = self.get_threshold(first, second)
        #print(t)
        if second.pos['z'] >= first.pos['z']:
            return False
        return self.distance(first,second) <= 2

    def get_described_position(self, description, protagonist):
        """ Returns the position/location described, e.g. "into the room", "near the box".
        (As opposed to an object described in relation to a location.) """
        obj = self.get_described_object(description['objectDescriptor'])
        if description['relation'] == 'behind':
            return self.behind(obj.pos, protagonist.pos)
        else:
            print(properties['relation'])

    def behind(self, position, reference):
        xdiff = position.x - reference.y
        ydiff = position.y - reference.y
        if abs(xdiff) > abs(ydiff):
            if xdiff>0:
                new = [position.x +3, position.y]
            elif xdiff<0:
                new = [position.x -3, position.y]
        elif abs(xdiff) < abs(ydiff):
            if ydiff>0:
                new = [position.x, position.y+3]
            elif ydiff<0:
                new = [position.x , position.y-3]
        elif abs(xdiff) == abs(ydiff):
            if ydiff>0:
                newy =  position.y+3
            elif ydiff<0:
                newy = position.y-3
            if xdiff>0:
                newx =  position.x+3
            elif xdiff<0:
                newx = position.x-3
            new = [newx, newy]
        new.append(0)
        return new




    def get_described_locations(self, candidates, description):
        obj = self.get_described_object(description['objectDescriptor'])
        if obj:
            locations = []
            if description['relation'] == 'near':
                locations = self.get_near(candidates, obj)
            elif description['relation'] == 'behind':
                # TODO: get_behind
                pass
            return locations
        else:
            return []




    def get_described_location(self, candidates, description, multiple=False):
        locations = self.get_described_locations(candidates, description)
        if multiple:
            return locations
        if len(locations) != 1:
            # TODO: what if there is more than one, what if there are none?
            return []
        else:
            return locations


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

    def match_push_move(self, objs, parameters):
        """ Question: how to know whether you're discussing acted upon or protagonist?
        E.g., "move to the box Robot2 pushed", vs. "move to the robot that pushed Box2"
        TODO: Figure this out.
        """
        description = parameters['causalProcess']['actedUpon']['objectDescriptor']
        obj = self.get_described_object(description)
        if obj in objs:
            return [obj]
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





    def get_described_object(self, description, multiple=False):
        objs = self.get_described_objects(description, multiple)
        if len(objs) == 1:
            self._recent = objs[0]
            return objs[0]
        elif len(objs) > 1:
            if "givenness" in description:
                if description['givenness'] == 'typeIdentifiable' or description['givenness'] == "distinct":
                    if self._recent in objs:
                        objs.remove(self._recent)
                    # TODO: do something better than just random choice, e.g. "is a box near the blue box" (really means ANY)
                    return random.choice(objs)
            elif self._wh:
                message = "More than one object matches the description of {}.".format(self.assemble_string(description))
                self.identification_failure(message)
                return None
            #print(objs)
            message = "Which '{}'?".format(self.assemble_string(description))
            # TODO: Tag n-tuple
            tagged = self.tag_ntuple(dict(self.ntuple), description)
            #self.request_clarification(self.ntuple, message)
            self.request_clarification(tagged, message)
            return None
        else:
            message = "Sorry, I don't know what the {} is.".format(self.assemble_string(description))
            self.identification_failure(message)
            return None

    def tag_ntuple(self, ntuple, description):
        """ Tags all ntuple keys with a "*" if the value matches DESCRIPTION. """
        new = {}
        for key, value in ntuple.items():
            #print(value)
            #print(description)
            if value == description:
                new["*" + key] = value
            elif type(value) == dict:
                #pass
                new[key] = self.tag_ntuple(value, description)
                # Tag ntuple on value
            else:
                new[key] = value
        return new


    def evaluate_scalar_attribute(self, attribute, value, objs, kind):
        if kind == "superlative":
            pass
        elif kind == "comparative":
            pass
        else:
            copy = []
            for obj in objs:
                if self.evaluate_individual_attribute(attribute, value, obj):
                    copy.append(obj)
            return copy

    def evaluate_individual_attribute(self, attribute, value, obj):
        ranges = self._ranges[obj.type][attribute]
        obj_attribute = getattr(obj, attribute)
        if value > .5:
            return obj_attribute >= (ranges[-1]-ranges[0])/2
        else:
            return obj_attribute <= (ranges[-1]-ranges[0])/2


    def get_heavyest(self, objs):
        heaviest = objs[0]
        returned = [heaviest]
        for i in objs:
            if float(i.weight) > heaviest.weight:
                heaviest = i
                returned = [heaviest]
            elif (float(i.weight) == heaviest.weight) and (i.name != heaviest.name):
                returned.append(i)
                # DO SOMETHING HERE ***
        return returned

    def get_lightest(self, objs):
        lightest = objs[0]
        returned = [lightest]
        for i in objs:
            if float(i.weight) < lightest.weight:
                lightest = i
                returned = [lightest]
            elif (float(i.weight) == lightest.weight) and (i.name != lightest.name):
                returned.append(i)
                # DO SOMETHING HERE ***
        return returned

    def get_biggest(self, objs):
        biggest = objs[0]
        returned = [biggest]
        for i in objs:
            if float(i.size) > biggest.size:
                biggest = i
                returned = [biggest]
            elif (float(i.size) == biggest.size) and (i.name != biggest.name):
                returned.append(i)
                # DO SOMETHING HERE ***
        return returned

    def get_smallest(self, objs):
        """ Returns the smallest object of input OBJS. If there are multiple smallest, it returns multiple. """
        smallest = objs[0]
        returned = [smallest]
        for i in objs:
            if float(i.size) < smallest.size:
                smallest = i
                returned = [smallest]
            elif (float(i.size) == smallest.size) and (i.name != smallest.name):
                returned.append(i)
        return returned




    def assemble_string(self, properties):
        """ Takes in properties and assembles a string, to format: "which blue box", etc. """
        ont = properties['type']
        attributes = ""
        for key, value in properties.items():   # Creates string of properties
            if key == "referent":
                return value[0].upper() + value.replace("_instance", "")[1:]
            if key == "color": # or key == "size":
                attributes += " " + value
            if key == "location":
                attributes += " "  + value
            elif key == "locationDescriptor":
                attributes += " " + str(ont) + " " + value["relation"] + " the " + self.assemble_string(value['objectDescriptor'])
                return attributes
        return str(attributes) + " " + str(ont)

    def query_be(self, parameters):
        if "specificWh" in parameters:
            return self.eval_wh(parameters, self.ntuple['return_type'])
        else:
            msg = "Yes." if self.evaluate_condition(parameters) else "No."
            return msg

    def query_be2(self, parameters):
        return self.query_be(parameters)

    def eval_wh(self, parameters, return_type):
        num, referentType = return_type.split("::")
        #protagonist = parameters['protagonist']
        #predication = parameters['state']
        dispatch = getattr(self, "eval_{}".format(parameters['specificWh']))
        dispatch(parameters, num)

    def eval_what(self, parameters, num):
        # What is the color of the box?
        # Protagonist=color, predication={identical: {box}}
        protagonist = parameters['protagonist']
        predication = parameters['state']
        if "type" not in protagonist['objectDescriptor']:
            obj = self.get_described_object(predication['identical']['objectDescriptor'])
            if obj:
                self.respond_to_query(message="a {}".format(str(obj.type)))
            else:
                self.identification_failure("Object {} not found.".format(self.assemble_string(predication['identical']['objectDescriptor'])))
        else:
            prop = protagonist['objectDescriptor']['type']
            obj = self.get_described_object(predication['identical']['objectDescriptor'])
            if obj:
                if hasattr(obj, prop):
                    value = getattr(obj, prop)
                    self.respond_to_query(message=str(value))
                else:
                    self.identification_failure("Object {} does not have the property {}.".format(obj.name, prop))

    def eval_where(self, parameters, num="singleton"):
        predication = parameters['state']
        #protagonist = parameters['protagonist']
        obj = self.get_described_object(predication['identical']['objectDescriptor'])
        if obj and len(obj) >= 1:
                message = "The position of the {} is: x:{}, y:{}".format(self.assemble_string(predication['identical']['objectDescriptor']), obj.pos['x'], obj.pos['y'])
                message = "The position of the {} is: ({}, {})".format(self.assemble_string(predication['identical']['objectDescriptor']), obj.pos['x'], obj.pos['y'])
                self.respond_to_query(message)

    def eval_which(self, parameters, num):
        copy = []
        predication = parameters['state']
        objs = self.get_described_objects(parameters['protagonist']['objectDescriptor'])
        negated = parameters['state']['negated']
        for obj in objs:
            if negated and not self.evaluate_obj_predication(obj, predication):
                copy.append(obj)
            elif (not negated) and self.evaluate_obj_predication(obj, predication):
            #elif (not negated) and self.evaluate_be(obj, predication):
                copy.append(obj)
        if len(copy) < 1:
            self.identification_failure("Failed to identify an object matching this description.")
        elif len(copy) >= 1:
            if num == "singleton" and len(copy) > 1:
                self.identification_failure(message="There is more than one item matching this description.")
            reply = ""
            index = 0
            while index < len(copy):
                reply += "{}".format(copy[index].name.replace("_instance", ""))
                if index < (len(copy) - 1):
                    reply += ", "
                index += 1
            self.respond_to_query(message=reply)


    def query_move(self, parameters):
        answer = self.evaluate_move(parameters)
        value, reason = answer['value'], answer['reason']
        msg = "Yes." if value else "No, {}.".format(reason)
        return msg

    def query_push_move(self, parameters):
        answer = self.evaluate_push_move(parameters)
        value, reason = answer['value'], answer['reason']
        msg = "Yes." if value else "No, {}.".format(reason)
        return msg

    def evaluate_can_push_move(self, parameters, negated=False):
        info = self.get_push_info(parameters)
        if not info['actedUpon']:
            return None
        #negated = self.eventFeatures['negated']
        protagonist = info['protagonist']
        distance = info['distance']['scaleDescriptor']['value']
        work = self.calculate_work(info['actedUpon'].weight + protagonist.weight, distance)
        if protagonist.fuel < work: # Check for fuel
            if negated:
                pass
            else:
                return {'value': protagonist.fuel >= work, 'reason': "not enough fuel"}
        if info['goal']:
            pass
        if info['heading']: # Should have at least default heading
            more_info = self.get_push_direction_info(info['heading']['headingDescriptor'], info['actedUpon'], distance)
            loc = info['actedUpon'].pos
            x2, y2 = more_info['x2'], more_info['y2']
            for obj in self.world:
                stripped = obj.replace("_instance", "")
                actual = self.world[obj]
                if self.is_between({'x': x2, 'y': y2}, loc, actual.pos) and info['actedUpon']['name']!= obj and protagonist['name'] != obj:
                    return {'value': False, 'reason': "{} is in the way".format(stripped)}
        return {'value': True, 'reason': "it is possible"}

    # TODO: Clean this up.
    def evaluate_push_move(self,parameters):
        info = self.get_push_info(parameters)
        # Check if you're asking whether a robot CAN push a box
        if self.eventFeatures and "modality" in self.eventFeatures and self.eventFeatures['modality'] == "can":
            negated = self.eventFeatures['negated']
            return self.evaluate_can_push_move(parameters, negated)
        else:
            # "did Robot1 push Box2?", etc.
            print(info)


    def is_between(self, a, b, c):
        crossproduct = (c['y'] - a['y']) * (b['x']-a['x']) - (c['x'] - a['x']) * (b['y'] - a['y'])
        if abs(crossproduct) > 0:
            return False
        dotproduct = (c['x']- a['x']) * (b['x'] - a['x']) + (c['y'] - a['y'])*(b['y'] - a['y'])
        if dotproduct < 0 : return False
        squaredlengthba = (b['x'] - a['x'])*(b['x'] - a['x']) + (b['y']- a['y'])*(b['y'] - a['y'])
        if dotproduct > squaredlengthba: return False
        return True

    def evaluate_move(self, parameters):
        info = self.get_move_info(parameters)
        if self.eventFeatures and "modality" in self.eventFeatures and self.eventFeatures['modality'] == "can":
            negated = self.eventFeatures['negated']
            mover = info['protagonist']
            destination = info['destination']
            distance = self.euclidean_distance(mover.pos, destination)
            work = self.calculate_work(mover.weight, distance)
            if mover.fuel < work:
                if negated:
                    pass
                else:
                    return {'value': False, 'reason': "not enough fuel"}
            for obj in self.world:
                stripped = obj.replace("_instance", "")
                actual = self.world[obj]
                #if actual.pos == destination:
                #    return {'value': False, 'reason': "{} is in the way".format(stripped)}
            return {'value': True, 'reason': "it can"}

    # Assumes force is in Newtons, for W=F*D equation
    def calculate_work(self, force, distance):
        return force*distance

    def calculate_lift_work(self, force, displacement):
        return force * displacement



    def evaluate_condition(self, parameters):
        #parameters = event['eventProcess']
        action = parameters['actionary']

        #protagonist = self.get_described_object(parameters['protagonist']['objectDescriptor'])
        dispatch = getattr(self, "evaluate_{}".format(action))
        answer = dispatch(parameters)
        value, reason = answer['value'], answer['reason']
        #if protagonist:
        negated = False
        if self.p_features and 'negated' in self.p_features:
            negated = self.p_features['negated']
        #negated = parameters['state']['negated']
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

    def evaluate_be2(self, parameters):
        return self.evaluate_be(parameters)

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
            # TO DO; color, size, name?


    # Assertions not yet implemented for robots
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

    # Conditional declaratives not yet implemented for robots
    def solve_conditional_declarative(self, ntuple):
        self.decoder.pprint_ntuple(ntuple)


    def move(self, mover, x, y, z=1.0, speed=2, tolerance=3, collide=False):
        print("{} is moving to ({}, {}, {}).".format(mover.name, x, y, z))
        mover.pos['x'] = x
        mover.pos['y'] = y
        mover.pos['z'] = z

    def bring(self, actor, information, final_destination, goal_object):
        # self.move(actor, information['actedUpon'].pos['x'], information['actedUpon'].pos['y'])
        # HACK
        #goal = self.get_described_object(goal_object['objectDescriptor']).grasp_pos
        #final_destination = {'x': goal['x'], 'y': goal['y']}
        self.move(actor, information['actedUpon'].pos['x'], information['actedUpon'].pos['y'])
        self.grasp(actor, information['actedUpon']['name'])
        self.move(actor, final_destination['x'], final_destination['y'])
        self.release(actor, information['actedUpon']['name'])

    def pickup(self, info):
        print(info['actedUpon'])
        print(info['actedUpon'].name)
        x, y = info['actedUpon'].grasp_pos['x'], info['actedUpon'].grasp_pos['y']
        #self.move(info['protagonist'], info['actedUpon'].pos['x'], info['actedUpon'].pos['x'])
        self.move(info['protagonist'], x, y)
        self.grasp(info['protagonist'], info['actedUpon'].name)

    def release(self, actor, label):
        print("{} is releasing the {}.".format(actor.name, label))

    def grasp(self, grasper, object_name):
        print("{} is grasping {}.".format(grasper.name, object_name))

if __name__ == "__main__":
    solver = BasicStarcraftProblemSolver(sys.argv[1:])
