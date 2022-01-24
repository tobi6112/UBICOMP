import logger from "../util/logger";
import { NodeCommand, NodeCommandPayload, NodeQualifier, State } from "../types";
import { publish } from "../net/mqtt";
import { nodeConfig } from "../util/config";



const isVirtualNode = (qualifier: NodeQualifier): boolean => nodeConfig[qualifier]?.virtual ?? false;
const virtualNodeStates: Record<NodeQualifier, State> = Object.keys(nodeConfig)
    .filter((key) => isVirtualNode(key))
    .reduce((acc, key) => ({ ...acc, [key]: 0 as State}), {});
const isVirtualStateUnequal = (qualifier: NodeQualifier, state: State): boolean => virtualNodeStates[qualifier] !== state;

const updateVirtualNodeState = (qualifier: NodeQualifier, state: State): void => {
    virtualNodeStates[qualifier] = state;
};

export const updateStateIfVirtualNode = (qualifier: NodeQualifier, state: State): void => {
    if(isVirtualNode(qualifier)) {
        if(isVirtualStateUnequal(qualifier, state)) {
            publish(`node/${qualifier}/state`, JSON.stringify({
                node: qualifier,
                state: state
            })); 
            updateVirtualNodeState(qualifier, state);
        }
    }
};

export const sendNodeCommand = (qualifier: NodeQualifier, command: NodeCommand): void => {
    logger.debug(`Sending Command ${command} to node ${qualifier}`);
    const payload: NodeCommandPayload = {
        command: command,
        node: qualifier
    };
    publish(`node/${qualifier}/set`, JSON.stringify(payload));
    
    if(isVirtualNode(qualifier)) {
        const lastState = virtualNodeStates[qualifier];
        if(lastState === undefined || lastState === null) throw new Error(`Couldn't find state for qualifier ${qualifier} although its defined as virtual`);
        const newState: State = command === 2 ? (lastState === 0 ? 1 : 0) : command;
        updateStateIfVirtualNode(qualifier, newState);
    }
};