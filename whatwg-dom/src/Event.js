import {
    caller,
    constructorErrorMessageInterceptor,
    functionInterceptor,
    getter,
    methodErrorMessageInterceptor,
    Platform,
    requireArgumentCountInterceptor,
    returnValueInterfaceInterceptor,
    validateThisImplementationInterceptor,
    returnArrayInterfaceInterceptor,
    illegalConstructor,
    illegalInvocation,
    validateConstructorTargetInterceptor
} from '@dragiyski/web-platform-core';
import { requireNewTargetInterceptor } from './interceptor.js';

const EventPhase = Object.assign(Object.create(), {
    NONE: 0,
    CAPTURING_PHASE: 1,
    AT_TARGET: 2,
    BUBBLING_PHASE: 3
});

class EventInit {
    constructor(dict) {
        if (dict == null) {
            dict = {};
        } else if (dict !== Object(dict)) {
            Platform.current().throw(new TypeError(`The provided value is not of type 'EventInit'.`));
        }
        for (const key of ['bubbles', 'cancelable', 'composed']) {
            this[key] = !!dict[key];
        }
    }
}

Object.setPrototypeOf(EventInit.prototype, null);

class Event {
    target = null;
    relatedTarget = null;
    touchTargetList = [];
    path = [];
    type = '';
    currentTarget = null;
    stopPropagation = false;
    stopImmediatePropagation = false;
    canceled = false;
    inPassiveListener = false;
    composed = false;
    initialized = false;
    dispatching = false;
    bubbles = false;
    cancelable = false;

    static legacyUnforgeables(event) {
        const platform = Platform.current();
        Object.defineProperty(platform.interfaceOf(event), 'isTrusted', {
            enumerable: true,
            get: platform.createNativeFunction(
                { name: 'isTrusted', length: 0 },
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().Event,
                    functionInterceptor(
                        function isTrusted() {
                            return this.isTrusted;
                        }
                    )
                )
            )
        });
    }

    /**
     * @param {*} type
     * @param {*} bubbles
     * @param {*} cancelable
     */
    initialize(type, bubbles, cancelable) {
        this.initialized = true;
        this.stopPropagation = false;
        this.stopImmediatePropagation = false;
        this.canceled = false;
        this.target = null;
        this.type = '' + type;
        this.bubbles = !!bubbles;
        this.cancelable = !!cancelable;
    }

    composedPath() {
        const composedPath = [];
        const path = this.path;
        if (path.length <= 0) {
            return composedPath;
        }
        const currentTarget = this.currentTarget;
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
            if (currentHiddenLevel < maxHiddenLevel) {
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
        index = currentTargetIndex + 1;
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

    setCanceledFlag() {
        if (this.cancelable && !this.inPassiveListener) {
            this.canceled = true;
        }
    }
}

function Constructor(platform, _, [type, eventInitDict], Implementation) {
    const realm = platform.getRealm();
    eventInitDict = new realm.EventInit(eventInitDict);
    const time = realm.performance?.now?.() ?? performance.now();
    const event = platform.call(realm.innerEventCreationSteps, null, Implementation, platform, time, eventInitDict);
    event.type = '' + type;
    return event;
}

/**
 * @param {Function} Implementation
 * @param {Platform} platform
 * @param {int} time
 * @param {EventInit} dictionary
 */
function innerEventCreationSteps(Implementation, platform, time, dictionary) {
    const interface_prototype = platform.interfaceOf(Implementation);
    if (interface_prototype !== Object(interface_prototype)) {
        throw illegalConstructor();
    }
    const implementation = new Implementation();
    const iface = Object.create(interface_prototype);
    platform.setImplementation(iface, implementation);
    if (typeof Implementation.legacyUnforgeables === 'function') {
        Implementation.legacyUnforgeables(implementation);
    }
    implementation.initialized = true;
    implementation.timeStamp = time;
    for (const member in dictionary) {
        if (member in implementation) {
            implementation[member] = dictionary[member];
        }
    }
    if (typeof Implementation.eventConstructingSteps === 'function') {
        Implementation.eventConstructingSteps(implementation, dictionary);
    }
    return implementation;
}

function createEvent(Implementation, platform, time = null) {
    const realm = platform.getRealm();
    const dictionary = new realm.EventInit();
    if (time == null) {
        time = realm.performance?.now?.() ?? performance.now();
    }
    const event = platform.call(realm.innerEventCreationSteps, null, Implementation, platform, time, dictionary);
    event.isTrusted = true;
    return event;
}

/**
 * @param {Platform} platform
 */
export default function install(platform) {
    const realm = platform.getRealm();
    realm.EventPhase = EventPhase;
    realm.EventInit = EventInit;
    realm.Event = Event;
    realm.innerEventCreationSteps = innerEventCreationSteps;
    realm.createEvent = createEvent;

    platform.realm.Event = platform.createInterface(
        realm.Event,
        { name: 'Event', length: 1 },
        returnValueInterfaceInterceptor(
            constructorErrorMessageInterceptor(
                'Event',
                requireNewTargetInterceptor(
                    validateConstructorTargetInterceptor(
                        () => Platform.current().getRealm().Event,
                        requireArgumentCountInterceptor(1, Constructor)
                    )
                )
            )
        )
    );

    for (const name of ['target', 'currentTarget']) {
        Object.defineProperty(platform.realm.Event.prototype, name, {
            configurable: true,
            enumerable: true,
            get: platform.createNativeFunction(
                { name, length: 0 },
                returnValueInterfaceInterceptor(
                    validateThisImplementationInterceptor(
                        () => Platform.current().getRealm().Event,
                        functionInterceptor(
                            getter(name)
                        )
                    )
                )
            )
        });
    }

    Object.defineProperty(platform.realm.Event.prototype, 'srcElement', {
        configurable: true,
        enumerable: true,
        get: platform.createNativeFunction(
            { name: 'srcElement', length: 0 },
            returnValueInterfaceInterceptor(
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().Event,
                    functionInterceptor(
                        getter('target')
                    )
                )
            )
        )
    });

    for (const name of ['type', 'eventPhase', 'bubbles', 'cancelable', 'composed', 'timeStamp']) {
        Object.defineProperty(platform.realm.Event.prototype, name, {
            configurable: true,
            enumerable: true,
            get: platform.createNativeFunction(
                { name, length: 0 },
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().Event,
                    functionInterceptor(
                        getter(name)
                    )
                )
            )
        });
    }

    Object.defineProperty(platform.realm.Event.prototype, 'composedPath', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            { name: 'composedPath', length: 0 },
            returnArrayInterfaceInterceptor(
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().Event,
                    functionInterceptor(
                        caller('composedPath')
                    )
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'stopPropagation', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            { name: 'stopPropagation', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function stopPropagation() {
                        this.stopPropagation = true;
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'cancelBubble', {
        configurable: true,
        enumerable: true,
        get: platform.createNativeFunction(
            { name: 'cancelBubble', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    getter('stopPropagation')
                )
            )
        ),
        set: platform.createNativeFunction(
            { name: 'cancelBubble', length: 1 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function cancelBubble(value) {
                        if (value) {
                            this.stopPropagation = true;
                        }
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'stopImmediatePropagation', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            { name: 'stopImmediatePropagation', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function stopImmediatePropagation() {
                        this.stopPropagation = true;
                        this.stopImmediatePropagation = true;
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'returnValue', {
        configurable: true,
        enumerable: true,
        get: platform.createNativeFunction(
            { name: 'returnValue', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function returnValue() {
                        return !this.canceled;
                    }
                )
            )
        ),
        set: platform.createNativeFunction(
            { name: 'returnValue', length: 1 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function returnValue(value) {
                        if (!value) {
                            this.setCanceledFlag();
                        }
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'preventDefault', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            { name: 'preventDefault', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function preventDefault() {
                        this.setCanceledFlag();
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'defaultPrevented', {
        configurable: true,
        enumerable: true,
        get: platform.createNativeFunction(
            { name: 'defaultPrevented', length: 0 },
            validateThisImplementationInterceptor(
                () => Platform.current().getRealm().Event,
                functionInterceptor(
                    function defaultPrevented() {
                        return !!this.canceled;
                    }
                )
            )
        )
    });

    Object.defineProperty(platform.realm.Event.prototype, 'initEvent', {
        configurable: true,
        enumerable: true,
        writable: true,
        value: platform.createNativeFunction(
            { name: 'initEvent', length: 0 },
            methodErrorMessageInterceptor(
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().Event,
                    requireArgumentCountInterceptor(
                        1,
                        functionInterceptor(
                            function initEvent(type, bubbles, cancelable) {
                                if (this.dispatching) {
                                    return;
                                }
                                this.initialize(type, bubbles, cancelable);
                            }
                        )
                    )
                )
            )
        )
    });

    // [Exposed=*]
    Object.defineProperty(platform.global, 'Event', {
        configurable: true,
        writable: true,
        value: platform.realm.Event
    });

    if (platform.is('Window')) {
        Object.defineProperty(platform.global, 'event', {
            configurable: true,
            get: platform.createNativeFunction(
                { name: 'event', length: 0 },
                returnValueInterfaceInterceptor(
                    function event(platform, self) {
                        if (self !== platform.global) {
                            throw illegalInvocation();
                        }
                        return platform.interfaceOf(platform.global).currentEvent;
                    }
                )
            ),
            set: platform.createNativeFunction(
                { name: 'event', length: 1 },
                function event(platform, self, [value]) {
                    if (self !== platform.global) {
                        throw illegalInvocation();
                    }
                    Object.defineProperty(platform.global, 'event', {
                        configurable: true,
                        enumerable: true,
                        writable: true,
                        value
                    });
                }
            )
        });
    }
}
