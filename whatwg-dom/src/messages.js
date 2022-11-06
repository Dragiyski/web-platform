import { messages } from "@dragiyski/web-platform-core";

export default Object.assign(Object.create(messages), {
    newOperatorRequired() {
        return `Please use the 'new' operator, this DOM object constructor cannot be called as a function.`;
    },
    eventAlreadyDispatched() {
        return 'The event is already being dispatched.';
    },
    invalidCast(type) {
        return `Failed to convert value to '${type}'.`;
    },
    failedToReadProeperty(name, type, message) {
        return `Failed to read the '${name}' property from '${type}': ${message}`;
    }
});
