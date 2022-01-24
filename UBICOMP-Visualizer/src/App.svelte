<script>
  import mqtt from 'mqtt/dist/mqtt.min';
  import {
    Container,
    Badge
  } from 'sveltestrap';
  import moment from 'moment';
import { compute_rest_props } from 'svelte/internal';
  moment.locale();
  export let name;
  let nodes = {
    "node-1": {
      state: 0
    },
    "node-2": {
      state: 0
    },
    "node-3": {
      state: 0
    },
    "node-4": {
      state: 0
    },
    "node-5": {
      state: 0
    },
    "node-6": {
      state: 0
    },
    "node-7": {
      state: 0
    },
    "node-8": {
      state: 0
    },
    "node-9": {
      state: 0
    }
  };

  let clientId = 'monitor_';
  if (DEVICE_ID) {
    clientId += DEVICE_ID;
  }
  clientId += '_' + Math.floor(Math.random() * 10000);
  let connected = false;
  const mqtt_options = {
    clientId: clientId,
    servers: [
      {
        host: MQTT_HOST,
        port: Number(MQTT_PORT),
        protocol: MQTT_PROTOCOL,
      },
    ],
    path: '/' + MQTT_PATH,
    clean: true,
    reconnectPeriod: 1000,
    connectTimeout: 30 * 1000,
    rejectUnauthorized: false,
  };
  if (MQTT_USERNAME) {
    mqtt_options.username = MQTT_USERNAME;
    mqtt_options.password = MQTT_PASSWORD;
  }
  console.log(mqtt_options);
  const url = `${MQTT_PROTOCOL}://${MQTT_HOST}:${MQTT_PORT}${mqtt_options.path}`;
  const onConnect = () => {
    console.log(`MQTT connected ${url}`);
    connected = client.connected;
    const topic = MQTT_SUBSCRIBE_TOPIC;
    client.subscribe(topic, function (err) {
      if (err) {
        console.error(err);
      } else {
        console.log(`MQTT subscribed on topic '${topic}'`);
      }
    });
  };

  const onMessage = (topic, message) => {
    const msg = message.toString();
    const time = new Date().getTime();
    console.log({ topic, message, msg, time })
    const { state, node } = JSON.parse(msg);

    if(node) {
      nodes = {
        ...nodes,
        ...({ [node]: state })
      };
    }
  };

  export const client = mqtt.connect(mqtt_options);
  client.on('connect', onConnect);
  client.on('message', onMessage);
  client.on('error', (err) => {
    console.log('MQTT', err);
    client.end();
    connected = client.connected;
  });
  client.on('close', () => {
    console.log('MQTT ' + clientId + ' disconnected');
    connected = client.connected;
  });
</script>

<style>
  .navbar-brand {
    text-transform: uppercase;
    font-weight: 100;
  }

  .logo {
    height: 4vh;
    width: 4vh;
  }
  .clear-fix {
    clear: both;
  }
  h3 {
    font-size: 2rem;
  }
  .small {
    font-size: 60%;
    padding-left: 0;
  }
  .topic {
    word-wrap: break-word;
  }
  .item {
    padding-top: 0.5rem;
  }
  .start,
  .end {
    width: 20%;
  }
  .url {
    width: 60%;
  }
  .end {
    flex-direction: row-reverse;
  }
  .icon {
    width: 2.5rem;
    display: inline-flex;
  }
  @media screen and (max-width: 480px) {
    h4,
    .navbar-brand {
      font-size: 1rem;
    }
    .url {
      font-size: 80%;
    }
  }

  .table thead th {
    font-size: 110%;
  }
  pre {
    margin-top: 0;
    margin-bottom: 0;
  }
  .card-body {
    padding: 1rem;
  }
</style>

<nav class="navbar navbar-light bg-light">
  <h4 class="text-center pt-1 text-dark">{name}</h4>
  <div class="form-inline">
    <Badge color={connected ? 'success' : 'danger'}>
      MQTT {connected ? 'connected' : 'disconnected'}
    </Badge>
  </div>
</nav>

<Container class="text-center">
  <div style="flex:1; display:flex; flex-flow:row; justify-content:space-between">
    {#each Object.entries(nodes) as [node, state]}
      <div>
        <svg viewBox="0 0 120 120" fill="{state === 1 ? 'green' : state === 0 ? 'red' : 'gray' }" version="1.1" xmlns="http://www.w3.org/2000/svg">
          <circle cx="60" cy="60" r="50"/>
        </svg>
        <p>{node}</p>
      </div>
    {/each}
  </div>
</Container>
