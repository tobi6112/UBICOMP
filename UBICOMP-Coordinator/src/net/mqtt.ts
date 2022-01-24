import * as mqtt from "mqtt";
import logger from "../util/logger";

import { MQTT_HOST, MQTT_PASSWORD, MQTT_PORT, MQTT_USER } from "../util/secrets";

type TopicCallback = (topic: string, payload: Buffer) => void;
type Topic = string;

type TopicHandlerRegister = Record<Topic, TopicCallback[]>; 

const client = mqtt.connect(`mqtt://${MQTT_HOST}`, {
    port: MQTT_PORT,
    username: MQTT_USER,
    password: MQTT_PASSWORD
});

const topicHandlers: TopicHandlerRegister = {};

const matchesTopicPattern = (topicPattern: string, topicToTest: string): boolean => {
    const regex = new RegExp(topicPattern.replace("+", ".+"));
    return regex.test(topicToTest);
};

client.on("connect", () => {
    logger.info("Successfully connected to MQTT Broker");
    for(const [topic, handlers] of Object.entries(topicHandlers)) {
        if(handlers.length > 1) {
            logger.debug(`Subscribing to MQTT Topic '${topic}'`);
            client.subscribe(topic);
        }
    }
});

client.on("error", (err) => {
    logger.error(`Could not connect to MQTT Broker: ${err}`);
});

client.on("message", (topic, payload) => {
    logger.debug(`Recieved Message on '${topic}' with '${payload}'`); 
    for(const [handlerTopic, callback] of Object.entries(topicHandlers)) {
        if(matchesTopicPattern(handlerTopic, topic)) {
            try {
                callback.forEach(cb => cb(topic, payload));
            } catch(err) {
                logger.error(`Error in Callback: ${err}`);
            }
        }
    }
});

export const registerTopicHandler = (topic: Topic, callback: TopicCallback): void => {
    const hasAlreadyCallbacks = topicHandlers[topic]?.length > 0 ?? false;
    if(hasAlreadyCallbacks) {
        topicHandlers[topic].push(callback);
    } else {
        topicHandlers[topic] = [callback];
    }

    client.subscribe(topic);
};

export const publish = (topic: string, message: string | Buffer): void => {
    client.publish(topic, message);
};