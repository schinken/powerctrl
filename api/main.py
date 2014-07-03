from flask import Flask, Response, render_template, request
from werkzeug.exceptions import NotFound
from collections import defaultdict
import powerctrl
import json

app = Flask(__name__)

# READ CONFIGURATION
settings_file = open('settings.json', 'r')
config = json.load(settings_file)

# init serial
serial_actor = powerctrl.SerialActor(config['server']['device'])

actor = powerctrl.DecoupledActor(serial_actor)
actor.daemon = True
actor.start()
    
devices = {}

for device in config['devices']:

    key = device['key']
    if key in devices:
        raise Exception('Duplicate key %s' % (key))

    devices[key] = powerctrl.Device(actor, device)

    if 'initial' in device:
        devices[key].set_status(device['initial'])
    else:
        devices[key].set_status(False)



def get_device(key):
    if not key in devices:
        raise NotFound({'message': "device not found"})

    return devices[key]


def json_response(data, http_code=200):
    return Response(response=json.dumps(data),
                    status=http_code,
                    mimetype="application/json")

def openhab_response(data, http_code=200):
    return Response(response=str(data),
                    status=http_code,
                    mimetype="text/plain")


@app.route("/device", methods=['GET'])
def devices_get():
    tmp ={}
    for key, device in devices.items():
        tmp[key] = device.serialize()

    return json_response(tmp)

@app.route("/device/<key>", methods=['GET'])
def device_get(key):
    return json_response(get_device(key).serialize())

@app.route("/device/<key>/status", methods=['GET'])
def device_get_status(key):
    device = get_device(key)

    if request.args.get('format') == 'openhab':
        return openhab_response('ON' if device.get_status() else 'OFF')

    return json_response(device.get_status())

@app.route("/device/<key>", methods=['POST'])
def device_post(key):
    device = get_device(key)
    device.set_status(True)

    return json_response(device.serialize())

@app.route("/device/<key>", methods=['DELETE'])
def device_delete(key):
    device = get_device(key)
    device.set_status(False)

    return json_response(device.serialize())



@app.route("/")
def page_main():
    return render_template('index.html')

if __name__ == "__main__":

    app.debug = True
    app.run(host='0.0.0.0')
