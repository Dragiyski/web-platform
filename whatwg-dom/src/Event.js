import { Platform, symbols } from '@dragiyski/web-platform-core';
import * as interceptors from './interceptor.js';
import { performance } from 'node:perf_hooks';
import { eventConstructingSteps } from './symbols.js';

const { concept, constructor } = symbols;

export default class Event {
    static [concept] = Object.assign(Object.create(null), {
        EventPhase: Object.assign(Object.create(null), {
            NONE: 0,
            CAPTURING_PHASE: 1,
            AT_TARGET: 2,
            BUBBLING_PHASE: 3
        }),
        EventInitDict(dict) {
            const eventInitDict = Object.create(null);
            for (const key in ['bubbles', 'cancelable', 'composed']) {
                eventInitDict[key] = !!dict[key];
            }
            return eventInitDict;
        },
        composedPathReturnValueInterceptor(callee) {
            return function interceptor(platform, ...rest) {
                const value = callee(platform, ...rest);
                const wrapped = new platform.primordials.Array();
                Reflect.apply(platform.primordials['Array.prototype.push'], wrapped, value.map(impl => platform.interfaceOf(impl) ?? impl));
                return wrapped;
            };
        },
        setCanceledFlag(event) {
            if (event[concept].cancelable && !event[concept].inPassiveListenerFlag) {
                event[concept].canceledFlag = true;
            }
        },
        initialize(event, type, bubbles, cancelable) {
            event[concept].initializedFlag = true;
            event[concept].stopPropagationFlag = false;
            event[concept].stopImmediatePropagationFlag = false;
            event[concept].canceledFlag = false;
            event[concept].isTrusted = false;
            event[concept].target = null;
            event[concept].type = '' + type;
            event[concept].bubbles = !!bubbles;
            event[concept].cancelable = !!cancelable;
        },
        create(Interface, platform = null, signaledAt = null) {
            if (platform == null) {
                try {
                    platform = Platform.current();
                } catch (e) {
                    if (!(e instanceof Platform.RuntimeError)) {
                        throw e;
                    }
                }
            }
            const dictionary = Object.create(null);
            if (signaledAt == null) {
                signaledAt = performance.timeOrigin + performance.now();
            }
            const event = this.innerEventCreationSteps(Interface, platform, signaledAt, dictionary);
            event[concept].isTrusted = true;
            return event;
        },
        innerEventCreationSteps(Interface, platform, time, dictionary) {
            // platform can be null if this class is used without a platform.
            // However, [constructor] symbol from the platform cannot be called,
            // the type must be initialized manually, as it will be initially an empty string.
            const event = new Interface();
            let timeOrigin = performance.timeOrigin;
            if (platform != null) {
                const eventInterface = Object.create(platform.interfaceOf(Interface.prototype));
                platform.setImplementation(eventInterface, event);
                if (platform.realm.performance?.[concept]?.timeOrigin != null) {
                    // In case performance API is simulated, timeOrigin can be shifted to "start of page" in a simulated browser.
                    timeOrigin = platform.realm.performance[concept].timeOrigin;
                }
            }
            event[concept].timeStamp = time - timeOrigin;
            for (const key in Object.keys(dictionary)) {
                if (key in event[concept]) {
                    event[concept][key] = dictionary[key];
                }
            }
            if (typeof platform?.realm?.[eventConstructingSteps] === 'function') {
                platform.realm[eventConstructingSteps](event, dictionary);
            }
            return event;
        }
    });

    [concept] = Object.assign(Object.create(null), {
        target: null,
        relatedTarget: null,
        path: [],
        type: '',
        currentTarget: null,
        eventPhase: this[concept].EventPhase.None,
        stopPropagationFlag: false,
        stopImmediatePropagationFlag: false,
        canceledFlag: false,
        inPassiveListenerFlag: false,
        composedFlag: false,
        initializedFlag: false,
        dispatchFlag: false,
        bubbles: false,
        cancelable: false,
        isTrusted: false,
        timeStamp: void 0
    });

    get type() {
        return this[concept].type;
    }

    get target() {
        return this[concept].target;
    }

    get srcElement() {
        return this[concept].target;
    }

    get currentTarget() {
        return this[concept].currentTarget;
    }

    composedPath() {
        const composedPath = [];
        const path = this[concept].path;
        if (path.length <= 0) {
            return composedPath;
        }
        const currentTarget = this[concept].currentTarget;
        composedPath.push(currentTarget);
        let currentTargetIndex = 0;
        let currentTargetHiddenSubtreeLevel = 0;
        let index = path.length - 1;
        while (index >= 0) {
            if (path[index].rootOfClosedTree) {
                ++currentTargetHiddenSubtreeLevel;
            }
            if (path[index].invocationTarget === currentTarget) {
                currentTargetIndex = index;
                break;
            }
            if (path[index].slotInClosedTree) {
                --currentTargetHiddenSubtreeLevel;
            }
            --index;
        }
        let currentHiddenLevel = currentTargetHiddenSubtreeLevel;
        let maxHiddenLevel = currentTargetHiddenSubtreeLevel;
        index = currentTargetIndex - 1;
        while (index >= 0) {
            if (path[index].rootOfClosedTree) {
                ++currentHiddenLevel;
            }
            if (currentHiddenLevel <= maxHiddenLevel) {
                composedPath.unshift(path[index].invocationTarget);
            }
            if (path[index].slotInClosedTree) {
                --currentHiddenLevel;
                if (currentHiddenLevel < maxHiddenLevel) {
                    maxHiddenLevel = currentHiddenLevel;
                }
            }
            --index;
        }
        currentHiddenLevel = currentTargetHiddenSubtreeLevel;
        maxHiddenLevel = currentTargetHiddenSubtreeLevel;
        index = currentTarget + 1;
        while (index < path.length) {
            if (path[index].slotInClosedTree) {
                ++currentHiddenLevel;
            }
            if (currentHiddenLevel < maxHiddenLevel) {
                composedPath.push(path[index].invocationTarget);
            }
            if (path[index].rootOfClosedTree) {
                --currentHiddenLevel;
                if (currentHiddenLevel < maxHiddenLevel) {
                    maxHiddenLevel = currentHiddenLevel;
                }
            }
            ++index;
        }
        return composedPath;
    }

    get eventPhase() {
        return this[concept].eventPhase;
    }

    stopPropagation() {
        this[concept].stopPropagationFlag = true;
    }

    get cancelBubble() {
        return !!this[concept].stopPropagationFlag;
    }

    set cancelBubble(value) {
        if (value) {
            this[concept].stopPropagationFlag = true;
        }
    }

    stopImmediatePropagation() {
        this[concept].stopPropagationFlag = true;
        this[concept].stopImmediatePropagationFlag = true;
    }

    get bubbles() {
        return this[concept].bubbles;
    }

    get cancelable() {
        return this[concept].cancelable;
    }

    get returnValue() {
        return !this[concept].canceledFlag;
    }

    set returnValue(value) {
        if (!value) {
            Event[concept].setCanceledFlag(this);
        }
    }

    preventDefault() {
        Event[concept].setCanceledFlag(this);
    }

    get defaultPrevented() {
        return !!this[concept].canceledFlag;
    }

    get composed() {
        return !!this[concept].composedFlag;
    }

    get isTrusted() {
        return !!this[concept].isTrusted;
    }

    get timeStamp() {
        return this[concept].timeStamp;
    }

    initEvent(type, bubbles, cancelable) {
        if (this[concept].dispatchFlag) {
            return;
        }
        Event[concept].initialize(this, type, bubbles, cancelable);
    }

    static [constructor](type, eventInitDict) {
        const platform = Platform.current();
        const event = this[concept].innerEventCreationSteps(
            this,
            platform,
            performance.timeOrigin + performance.now(),
            this[concept].EventInitDict(eventInitDict)
        );
        event[concept].type = '' + type;
        return event;
    }
}

export function install(platform) {
    const {
        returnValueInterfaceInterceptor,
        validateThisImplementationInterceptor,
        functionInterceptor,
        getter,
        setter,
        caller
    } = interceptors.core;
    const Interface = platform.realm.Event = platform.createInterface(
        Event,
        returnValueInterfaceInterceptor(
            Event[constructor]
        ),
        {
            name: 'Event',
            length: 1
        }
    );
    for (const name of ['type', 'eventPhase', 'bubbles', 'cancelable', 'defaultPrevented', 'composed', 'timeStamp']) {
        Object.defineProperty(Interface.prototype, name, {
            configurable: true,
            enumerable: true,
            get: platform.createNativeFunction(
                validateThisImplementationInterceptor(
                    functionInterceptor(
                        getter(name)
                    ),
                    Platform.realm.Event
                ),
                { name, constructor: false }
            )
        });
    }
    for (const name of ['target', 'currentTarget', 'srcElement']) {
        Object.defineProperty(Interface.prototype, name, {
            configurable: true,
            enumerable: true,
            get: platform.createNativeFunction(
                returnValueInterfaceInterceptor(
                    validateThisImplementationInterceptor(
                        functionInterceptor(
                            getter(name)
                        ),
                        Platform.realm.Event
                    )
                ),
                { name, constructor: false }
            )
        });
    }

    for (const name of ['cancelBubble', 'returnValue']) {
        Object.defineProperty(Interface.prototype, name, {
            configurable: true,
            enumerable: true,
            get: platform.createNativeFunction(
                validateThisImplementationInterceptor(
                    functionInterceptor(
                        getter(name)
                    ),
                    Platform.realm.Event
                ),
                { name, constructor: false }
            ),
            set: platform.createNativeFunction(
                validateThisImplementationInterceptor(
                    functionInterceptor(
                        setter(name)
                    ),
                    Platform.realm.Event
                ),
                { name, constructor: false }
            )
        });
    }

    for (const name of ['stopPropagation', 'stopImmediatePropagation', 'preventDefault']) {
        Object.defineProperty(Interface.prototype, name, {
            configurable: true,
            enumerable: true,
            writable: true,
            value: platform.createNativeFunction(
                validateThisImplementationInterceptor(
                    functionInterceptor(
                        caller(name)
                    ),
                    Platform.realm.Event
                ),
                { name, constructor: false }
            )
        });
    }

    Object.defineProperty(Interface.prototype, 'composedPath', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            Event[concept].composedPathReturnValueInterceptor(
                validateThisImplementationInterceptor(
                    functionInterceptor(
                        caller('composedPath')
                    ),
                    Platform.realm.Event
                )
            ),
            { name: 'composedPath', constructor: false }
        )
    });

    // [Exposed=*]
    Object.defineProperty(platform.global, 'Event', {
        configurable: true,
        enumerable: false,
        writable: true,
        value: Interface
    });
}
