# Oven automation

### Setup in HA

* Enable integration Scheduler (the pink one with the box icon): http://localhost:8123/config/integrations/dashboard

### Requirements

1. Ramp 
2. Soak

Example:

1. Ramp 300C p/h to 1150 and hold for 30
2. Ramp 200 p/h to 1250 and hold for 0
3. Tamp

Ramp must be implemented on esphome side


### Ideas

Serverside scheduler with profiles, send mqtt message to esphome


Clientside:

Esphome receives the schedule over MQTT and executes it all by itself

Advantages:
* No need for an external server, might be completly possible on the ESP
* Light and easy

Disadvantages:
* Schedule logic must be all be in some lambda operation


### Local dev

1. Go to 100 ASAP (should take 1 interval in testing), hold 1 minute
2. Go to 200 with 3000c/u == 100 p/m so should take 2 minutes, then hold two minutes
3. Cooldown with 3000/u == 100p/m so should be at zero in 2 minutes

```
curl -X GET http://glas-kiln.local/kiln/schedule
curl -X POST http://glas-kiln.local/kiln/schedule \
    -H "Content-Type: application/json" \
    -d '{"name":"test","schedule":[[999999,100,1],[3000,200,2],[3000,0,0]]}'
```

### States

- heating, target_temperature is getting updates each 10s until step_target_temp is reached
- hold, 

### UI

Not using sveltekit but svelte + spa-router due this issue:
https://github.com/sveltejs/kit/issues/3882

## Credits/Inspiration

* [jbruce12000/kiln-controller](https://github.com/jbruce12000/kiln-controller/tree/master/public)
* [libretiny-eu/esphome-kickstart](https://github.com/libretiny-eu/esphome-kickstart/tree/master/components/hub_api)
* [ssieb/custom_components](https://github.com/ssieb/custom_components/blob/master/components/web_handler/)