
import { nodeConfig, globalConfig } from "./util/config";
import { registerTopicHandler } from "./net/mqtt";
import { on_ir_message } from "./sensor/ir";
import { on_nfc_message } from "./sensor/nfc";
import { on_intent_message } from "./sensor/speech";
import logger from "./util/logger";
import { NodeStateMessagePayload } from "./types";
import { updateStateIfVirtualNode } from "./node/node";                

const on_state_message = (topic: string, payload: Buffer): void => {
    const message: NodeStateMessagePayload = JSON.parse(payload.toString());
    updateStateIfVirtualNode(message.node, message.state);
};

registerTopicHandler("node/+/sensor/ir", on_ir_message);
registerTopicHandler("node/+/sensor/nfc", on_nfc_message);
registerTopicHandler(globalConfig.speech.intentTopic, on_intent_message);
registerTopicHandler("node/+/state", on_state_message);