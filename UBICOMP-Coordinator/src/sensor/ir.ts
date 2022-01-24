import logger from "../util/logger";
import { IRCode, IRMessagePayload, NodeQualifier } from "../types";
import { nodeConfig } from "../util/config";
import { globalConfig } from "../util/config";
import { sendNodeCommand } from "../node/node";
import { extractNodeFromTopic } from "./common";


const getAffectedNodes = (code: IRCode): NodeQualifier[] => Object.keys(nodeConfig).filter(key => nodeConfig[key].ir?.toggleCode === code);

const handle = (nodeQualifier: NodeQualifier, message: IRMessagePayload): void => {
    const { code } = message;    
    logger.debug(`Recieved IR Message from node ${nodeQualifier} with IR Code ${code}`);

    const isSelectiveOff = globalConfig.ir.selectiveOff === code;
    const isSelectiveOn = globalConfig.ir.selectiveOn === code;
    
    if(isSelectiveOff) {
        logger.debug(`Performing selective off defined for code ${code}`);
        sendNodeCommand(nodeQualifier, 0);
        return;
    }
    if(isSelectiveOn) {
        logger.debug(`Performing selective on defined for code ${code}`);
        sendNodeCommand(nodeQualifier, 1);
        return;
    }

    const affectedNodes = getAffectedNodes(code);
    logger.debug(`The IR event affects the nodes '${affectedNodes.join(", ")}'`);
    affectedNodes.forEach(n => sendNodeCommand(n[0], 2));
};

export const on_ir_message = (topic: string, message: Buffer): void => {
    const node = extractNodeFromTopic(topic);
    const parsedMessage = JSON.parse(message.toString());
    handle(node, parsedMessage);
};



// ------------------------------------------------------------------------------------

// I don't know where to put it, but don't want to throw away, since
// It's a pretty nice lookup table...
/*const IR_REMOTE: IRRemoteCodes = {
    "IR_DIM_UP": 0xFF906F,
    "IR_DIM_DOWN": 0xFFB847,
    "IR_CODE_ON": 0xFFB04F,
    "IR_CODE_OFF": 0xFFF807,

    "IR_CODE_RED_0": 0xFF9867,
    "IR_CODE_GREEN_0": 0xFFD827,
    "IR_CODE_BLUE_0": 0xFF8877,
    "IR_CODE_WHITE": 0xFFA857,

    "IR_CODE_RED_1": 0xFFE817,
    "IR_CODE_GREEN_1": 0xFF48B7,
    "IR_CODE_BLUE_1": 0xFF6897,

    "IR_CODE_RED_2": 0xFF02FD,
    "IR_CODE_GREEN_2": 0xFF32CD,
    "IR_CODE_BLUE_2": 0xFF20DF,

    "IR_CODE_RED_3": 0xFF50AF,
    "IR_CODE_GREEN_3": 0xFF7887,
    "IR_CODE_BLUE_3": 0xFF708F,

    "IR_CODE_RED_4": 0xFF38C7,
    "IR_CODE_GREEN_4": 0xFF28D7,
    "IR_CODE_BLUE_4": 0xFFF00F,

    "IR_CODE_FLASH": 0xFFB24D,
    "IR_CODE_STROBE": 0xFF00FF,
    "IR_CODE_FADE": 0xFF58A7,
    "IR_CODE_SMOOTH": 0xFF30CF,
};*/