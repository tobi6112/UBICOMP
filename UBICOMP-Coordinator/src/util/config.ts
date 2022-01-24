import * as fs from "fs";
import { IRCode, NfcUid, NodeQualifier } from "../types";

type NodeIRConfig = {
    toggleCode: IRCode;
};

type NodeSoundConfig = {

};

type NodeNfcConfig = {
    toggleUid: NfcUid;
};

type NodeSpeechConfig = {
    additionalQualifiers?: string[];
};

export type NodeConfig = {
    ir?: NodeIRConfig;
    sound?: NodeSoundConfig;
    nfc?: NodeNfcConfig;
    speech?: NodeSpeechConfig;
    virtual?: boolean;
};

type GlobalIRConfig = {
    selectiveOn?: IRCode;
    selectiveOff?: IRCode;
};

type GlobalSpeechConfig = {
    intentTopic: string;
};

type GlobalConfig = {
    ir: GlobalIRConfig;
    speech: GlobalSpeechConfig;
}

const CONFIG_FILE = "./config.json";

const config = JSON.parse(fs.readFileSync(CONFIG_FILE).toString());

export const nodeConfig: Record<NodeQualifier, NodeConfig> = config.nodes;

export const globalConfig: GlobalConfig = config.global;