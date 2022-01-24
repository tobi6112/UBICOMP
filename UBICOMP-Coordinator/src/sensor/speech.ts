import { nodeConfig } from "../util/config";
import logger from "../util/logger";
import { NodeCommand, NodeQualifier, ToggleNodeIntentPayload } from "../types";
import { sendNodeCommand } from "../node/node";

const getAffectedNodes = (value: string): NodeQualifier[] => Object.keys(nodeConfig).filter(key => key === value || nodeConfig[key]?.speech?.additionalQualifiers.includes(value));

const handle = (message: ToggleNodeIntentPayload): void => {
    const node = message.slots.find(s => s.entity === "node")?.value.value as NodeQualifier;
    const state = Number(message.slots.find(s => s.entity === "state").value.value) as NodeCommand;
    logger.debug(`Recieved Intent for node ${node} with state ${state}`);

    const affectedNodes = getAffectedNodes(node);
    logger.debug(`The Intent affects the nodes '${affectedNodes.join(",")}'`);
    
    affectedNodes.forEach(node => sendNodeCommand(node, state));
};

export const on_intent_message = (topic: string, message: Buffer): void => {
    handle(JSON.parse(message.toString()));
};