import logger from "../util/logger";
import { NodeQualifier } from "../types";

export const extractNodeFromTopic = (topic: string): NodeQualifier => { 
    const NODE_SENSOR_TOPIC_REGEX = /node\/(.+)\/sensor\/(.+)/i;
    const match = topic.match(NODE_SENSOR_TOPIC_REGEX);
    
    const node = match[1];

    logger.debug(`Extracted Params from topic '${topic}': Node: '${node}'`);
    return node;
};