import { nodeConfig } from "../util/config";
import { NfcMessagePayload, NfcUid, NodeQualifier } from "../types";
import logger from "../util/logger";
import { sendNodeCommand } from "../node/node";
import { extractNodeFromTopic } from "./common";

const getAffectedNodes = (uid: NfcUid): NodeQualifier[] => Object.keys(nodeConfig).filter(key => nodeConfig[key]?.nfc?.toggleUid === uid);

const handle = (node: NodeQualifier, payload: NfcMessagePayload): void => {
    const { uid } = payload;
    logger.debug(`Recieved NFC Message for UID ${uid}`);

    const affectedNodes = getAffectedNodes(uid);
    logger.debug(`The UID affects the nodes '${affectedNodes.join(", ")}'`);
    
    affectedNodes.forEach(node => {
        sendNodeCommand(node, 2);
    });
};

export const on_nfc_message = (topic: string, payload: Buffer): void => {
    const node = extractNodeFromTopic(topic);
    const parsedMessage = JSON.parse(payload.toString());
    handle(node, parsedMessage);
};