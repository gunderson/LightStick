import * as url from 'url';

// var osc = require("osc");
import * as OSC from 'osc';
let osc = OSC.default;

// var Path = require("path");
import * as path from 'path';

// var pug = require("pug");
import * as pug from 'pug';

import * as ArtKit from "art-kit";

import {
    networkInterfaces
} from 'os';

import * as bj from 'bonjour';
let bonjour = bj.default;
bonjour = bonjour()



// ------------------------------------------------------------------------------
// Setup Network

// const {
    // networkInterfaces
// } = require('os');

const nets = networkInterfaces();
const networkResults = Object.create(null); // Or just '{}', an empty object

for (const name of Object.keys(nets)) {
    for (const net of nets[name]) {
        // Skip over non-IPv4 and internal (i.e. 127.0.0.1) addresses
        if (net.family === 'IPv4' && !net.internal) {
            if (!networkResults[name]) {
                networkResults[name] = [];
            }
            networkResults[name].push(net.address);
        }
    }
}

console.log("network address: ", networkResults.Ethernet[0]);
// ------------------------------------------------------------------------------
// MDNS Server

 
// advertise an HTTP server on port 3000
bonjour.publish({
    name: 'displayhost', 
    host: 'displayhost.local', 
    type: 'http', 
    port: 3000 
})
 

// browse for all http services
bonjour.find({ type: 'http' }, function (service) {
    // console.log('Found an HTTP server:', service)
  })

// ------------------------------------------------------------------------------
// mdns browser



// ------------------------------------------------------------------------------
// Setup Server

// const Hapi = require('@hapi/hapi');
import * as Hapi from '@hapi/hapi';

// const Hoek = require('@hapi/hoek');
import * as Hoek from '@hapi/hoek';

import * as Vision from '@hapi/vision';
// import { update } from 'lodash';

// START WEB SERVER

const init = async () => {

    const server = Hapi.server({
        port: 3000,
        // host: 'displayhost.local'
    });

    
    await server.register(Vision);
    setupRoutes(server);
    setupPreprocessors(server);


    await server.start();


    console.log('Server running on %s', server.info.uri);
};

process.on('unhandledRejection', (err) => {
    console.log(err);
    process.exit(1);
});

init();

// Setup pre-processors

function setupPreprocessors(server){
    server.views({
        engines: {
            html: pug,
            pug: pug
        },
        relativeTo: url.fileURLToPath(new URL('.', import.meta.url)),
        path: "src/pug"
    });
}

// Setup Routes

function setupRoutes(server) {

    server.route({
        method: 'GET',
        path: '/control-panel',
        options: {
            handler: (req, h) => {
                return h.view('control-panel.pug', {});
            }
        }
    });

    server.route({
        method: 'POST',
        path: '/update-control',
        handler: function (request, h) {
            
            return 'Update Control!';
        }
    })

    server.route({
        method: 'GET',
        path: '/hello',
        handler: function (request, h) {

            return 'Hello World!';
        }
    });
    server.route({
        method: 'POST',
        path: '/register',
        handler: registerDevice
    })
    
    server.route({
        method: 'GET',
        path: '/flood',
        handler: flood
    })
    server.route({
        method: 'GET',
        path: '/fade',
        handler: fade
    })
}

// ------------------------------------------------------------------------------
// HANDLE Devices

const devices = [];

var udpPort = new osc.UDPPort({
    localAddress: "0.0.0.0",
    localPort: 57120,
    metadata: true
});

udpPort.on("ready", function () {
    console.log("Listening for OSC over UDP.");

});

udpPort.on("message", function (oscMessage) {
    console.log(oscMessage);
});

udpPort.on("error", function (err) {
    console.log(err);
});

udpPort.open();




// ------------------------------------------------------------------------------
// Animation
let player = new ArtKit.AnimationPlayer({
    loop: true,
    update: update,
    draw: draw
});

const PATTERNS = {
    "SWEEP": sweep
}

let frameRate = 25;
let frameDuration = 1000 / frameRate;
let frameIndex = 0;

let pattern = sweep;

let frameTimer;

player.fps = 25;


function playAnimation(){
    player.play();
}

function stopAnimation(){
    player.stop();
}

var lastDrawTime = Date.now();

function update(){
    pattern(frameIndex++);
}

function draw(){
    devices.forEach(device => device.draw(frameIndex))
    let drawDelta = Date.now() - lastDrawTime;
    // console.log("drawDelta", drawDelta)
    lastDrawTime = Date.now();
}

// ------------------------------------------------------------------------------
// Calibration

// manual calibration

let scenePhoto

function ingestScenePhoto(){

}

function updateDevice(deviceUptates){
    // TODO validate data?
    let keys = deviceUptates.keys;
    let device = devices.filter((device) => device.id === deviceUptates.id);
    keys.forEach((updatedKey) => {
        device[key] = deviceUptates[key];
    });
}

// auto calibration

function floodById(){

}

function submitPhtot(){

}

function evaluatePhotos(){
    let minX = 0.5;
    let maxX = 0.5;
    let minY = 0.5;
    let maxY = 0.5;
    // find constraints
    // for each device
        //threshold the photo
        //find brightest pixel top

}


// ------------------------------------------------------------------------------
// LED Patterns

function sweep(frameIndex){
    devices.forEach((device) => {
        let litIndex = frameIndex % device.numLEDs;
        // console.log('Sweep:: led count: ' + device.numLEDs, device.leds.length)
        device.updateLEDs(device.leds.map((color, index) => litIndex == index ? 0xff11ff : 0));
    });
}

function flood(request, h){
    let color = parseInt(request.query.color, 16);
   
    devices.forEach((device) => {
        device.flood(color);
    });
    return true;
}

function fade(request, h){
    let color = parseInt(request.query.color, 16);
   
    destColor = color;
    fadeTick = 0;
    fadeTimer = setInterval(onFadeTick, 1);
    return true;
}

function registerDevice(request, h) {
    console.log("Incoming registration");
    console.log(request.query);

    // TODO: validate query before continuing

    let address = request.query.address;
    let port = request.query.port;
    let numLEDs = request.query.numleds;

    if (devices.reduce((alreadyExists, device) => address == device.address || alreadyExists, false)) return 0;

    let newDevice = new LEDDevice(address, port, numLEDs);
    devices.push(newDevice);
    playAnimation();

    return 1;
}

let startColor = 0;
let destColor = 0;
let fadeTick = 0;
let fadeDuration = 1000;

let fadeTimer;

function onFadeTick(){
    let t = fadeTick / fadeDuration;

    let startR = startColor >> 16;
    let startG = startColor >> 8 & 0xff;
    let startB = startColor & 0xff;
    let destR = destColor >> 16;
    let destG = destColor >> 8 & 0xff;
    let destB = destColor & 0xff;

    function lerp(start, end, t){
        return (end - start) * t + start;
    }

    let color = lerp(startR, destR, t) << 16 | lerp(startG, destG, t) << 8 | lerp(startB, destB, t);

    devices.forEach((dev) => {
        dev.flood(color);
    });


    if (fadeTick++ == fadeDuration){
        clearInterval(fadeTimer);
    }
}


class LEDDevice {
    constructor(address, port, numLEDs) {
        this.address = address;
        this.port = parseInt(port);
        this.numLEDs = parseInt(numLEDs);
        this.leds = [];
        this.photo;
        while(this.leds.length < this.numLEDs){
            this.leds.push(0);
        }
        console.log('LEDDevice:: led count: ' + this.numLEDs, this.leds.length)
        console.log(`LEDDevice:: ${this.address}:${this.port}`);

    }

    // flood(color){
    //     let r = color >> 16;
    //     let g = color >> 8 & 0xff;
    //     let b = color & 0xff;

    //     udpPort.send({
    //         address: "/led",
    //         args: [
    //             {
    //                 type: "b",
    //                 value: new Uint8Array([r,g,b])
    //             }
    //         ]
    //     }, this.address, this.port);
    // }

    updateLEDs(colorArray) {
        this.leds = colorArray;
        // send a request to the device to update the colors

        colorArray = colorArray.reduce((arr, color) => arr.concat(color >> 16, (color >> 8) & 0xff, color & 0xff), []);
        // console.log(`numLEDs: ${this.numLEDs}`);
        // console.log(`LED Array length: ${this.leds.length}`);

        // console.log("colorArray", colorArray)

        udpPort.send({
            address: "/led",
            args: [
                {
                    type: "b",
                    value: new Uint8Array(colorArray)
                }
            ]
        }, this.address, this.port);

        

        // console.log("/led",this.address, this.port);
        // fetch(`http://${this.address}/update`, {
        //         method: 'POST',
        //         body: JSON.stringify(colorArray)
        //     })
        //     .then(res => res.json()) // expecting a json response
        //     .then(json => console.log(json));
    }

    draw(frameIndex){
        udpPort.send({
            address: "/draw",
            args: [
                {
                    type: "i",
                    value: frameIndex
                }
            ]
        }, this.address, this.port);

        // console.log("/draw",this.address, this.port);
    }

    flood(color) {
        let colorArray = this.leds.map(() => color);
        this.updateLEDs(colorArray);
    }
}

playAnimation();
