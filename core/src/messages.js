export default Object.assign(Object.create(null), {
    illegalConstructor() {
        return 'Illegal constructor';
    },
    illegalInvocation() {
        return 'Illegal invocation';
    },
    constructorFailedToExecute(className) {
        return `Failed to construct '${className}'`;
    },
    methodFailedToExecute(className, methodName) {
        return `Failed to execute '${className}' on '${methodName}'`;
    },
    invalidArgumentType(index, requiredType) {
        return `parameter ${index + 1} is not of type '${requiredType}'.`;
    },
    insufficientArgumentCount(count, required) {
        return `${required} argument required, but only ${count} present.`;
    }
});
