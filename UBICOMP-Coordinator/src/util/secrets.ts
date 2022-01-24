import logger from "./logger";
import dotenv from "dotenv";
import fs from "fs";

if (fs.existsSync(".env")) {
    logger.debug("Using .env file to supply config environment variables");
    dotenv.config({ path: ".env" });
} 
export const ENVIRONMENT = process.env.NODE_ENV;
const prod = ENVIRONMENT === "production";

export const MQTT_HOST: string = process.env["MQTT_HOST"];
export const MQTT_PORT: number = Number(process.env["MQTT_PORT"]) ?? 1883;
export const MQTT_USER: string | undefined = process.env["MQTT_USER"] ?? undefined;
export const MQTT_PASSWORD: string | undefined = process.env["MQTT_PASSWORD"] ?? undefined;

if (!MQTT_HOST) {
    logger.error("No MQTT Host. Set MQTT_HOST environment variable.");
    process.exit(1);
}
