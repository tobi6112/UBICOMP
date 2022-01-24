export type NodeQualifier = string;
export type NodeCommand = ENABLE | DISABLE | TOGGLE;
export type ENABLE = 1;
export type DISABLE = 0;
export type TOGGLE = 2;

type SensorType = "ir" | "nfc";
type State = 1 | 0;

interface NodeSensorTopicParams {
    node: NodeQualifier,
    sensor: SensorType
}

// type EnableCommand = 0;
// type DisableCommand = 1;

// type ToggleCommand = 2;
// type NodeCommand = EnableCommand | DisableCommand | ToggleCommand;
type NodeCommandPayload = {
    node: NodeQualifier,
    command: NodeCommand
}

type TopicHandler = {
    onMessage: (topic: string, payload: Buffer) => void;
};

type TopicHandlerRegistry = Record<string, TopicHandler>;

type NfcUid = string;
interface NfcMessagePayload {
    uid: NfcUid;
    type: string;
}

interface IRMessagePayload {
    code: IRCode;
    command: number;
}

interface NodeStateMessagePayload {
    node: NodeQualifier;
    state: State;
}

interface ToggleNodeIntentNodeEntity extends IntentNodeEntity {
    entity: "node"
}

interface ToggleNodeIntentStateEntity extends IntentNodeEntity {
    entity: "state"
}

interface IntentNodeEntity {
    value: {
        kind: string,
        value: string
    },
    slotName: string
}

interface ToggleNodeIntentPayload {
    intent: {
        intentName: string,
        confidenceScore: number
    },
    slots: (ToggleNodeIntentNodeEntity | ToggleNodeIntentStateEntity)[]
}

type IRRemoteKey = "IR_DIM_UP" | "IR_DIM_DOWN" | "IR_CODE_ON" | "IR_CODE_OFF" | 
                "IR_CODE_RED_0" | "IR_CODE_GREEN_0" | "IR_CODE_BLUE_0" | "IR_CODE_WHITE" | 
                "IR_CODE_RED_1" | "IR_CODE_GREEN_1" | "IR_CODE_BLUE_1" | 
                "IR_CODE_RED_2" | "IR_CODE_GREEN_2" | "IR_CODE_BLUE_2" | 
                "IR_CODE_RED_3" | "IR_CODE_GREEN_3" | "IR_CODE_BLUE_3" | 
                "IR_CODE_RED_4" | "IR_CODE_GREEN_4" | "IR_CODE_BLUE_4" | 
                "IR_CODE_FLASH" | "IR_CODE_STROBE" | "IR_CODE_FADE" | "IR_CODE_SMOOTH";



type IRCode = number;
type IRRemoteCodes = Record<IRRemoteKey, IRCode>;

// TODO: Implement
type SelectionStrategy = "MONO" | "POLY";


