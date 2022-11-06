import {
    constructorErrorMessageInterceptor,
    functionInterceptor,
    methodErrorMessageInterceptor,
    Platform, requireArgumentCountInterceptor, requireArgumentTypeInterceptor, returnValueInterfaceInterceptor, validateConstructorTargetInterceptor, validateThisImplementationInterceptor
} from '@dragiyski/web-platform-core';
import { requireNewTargetInterceptor } from './interceptor.js';
import { messages } from './messages.js';

class EventTarget {
    eventListenerList = [];

    getTheParent(event) {
        return null;
    }

    static defaultPassiveValue(type, eventTarget) {
        if (['touchstart', 'touchmove', 'wheel', 'mousewheel'].indexOf(type) < 0) {
            return false;
        }
        const platform = Platform.current();
        if (eventTarget === platform.global && platform.is('Window')) {
            return true;
        }
        const realm = platform.getRealm();
        if (realm.Node?.isNode?.(eventTarget)) {
            if (
                eventTarget.nodeDocument === eventTarget ||
                eventTarget.nodeDocument?.documentElement === eventTarget ||
                eventTarget.nodeDocument?.bodyElement === eventTarget
            ) {
                return true;
            }
        }
        return false;
    }

    /**
     * @param {EventListener} listener
     */
    addEventListener(listener) {
        if (listener.signal?.aborted) {
            return;
        }
        if (listener.callback == null) {
            return;
        }
        if (listener.passive == null) {
            listener.passive = this.defaultPassiveValue(listener.type, this);
        }
        if (!this.eventListenerList.some(
            item => item.callback === listener.callback && item.type === listener.type && item.capture === listener.capture
        )) {
            this.eventListenerList.push(listener);
        }
        if (listener.signal != null) {
            listener.signal.add(() => { this.removeEventListener(this, listener); });
        }
    }

    /**
     * @param {EventListener} listener
     */
    removeEventListener(listener) {
        listener.removed = true;
        const index = this.eventListenerList.indexOf(listener);
        if (index >= 0) {
            this.eventListenerList.splice(index, 1);
        }
    }

    removeAllEventListeners() {
        this.eventListenerList.forEach(listener => {
            listener.removed = true;
        });
        this.eventListenerList.length = 0;
    }
}

function convertToAbortSignal(value) {
    const platform = Platform.current();
    const realm = platform.getRealm();
    if (typeof realm.AbortSignal === 'function') {
        const AbortSignal = realm.AbortSignal;
        if (value instanceof AbortSignal) {
            return value;
        }
        const signal = platform.implementationOf(value);
        if (signal instanceof AbortSignal) {
            return signal;
        }
    }
    platform.throw(new TypeError(messages.invalidCast('AbortSignal')));
}

class EventListenerOptions {
    constructor(options) {
        if (options === Object(options)) {
            this.capture = !!options.capture;
        }
        this.capture = !!options;
    }
}

class AddEventListenerOptions extends EventListenerOptions {
    constructor(options) {
        super(options);
        this.once = false;
        this.passive = null;
        this.signal = null;
        if (options === Object(options)) {
            this.once = !!options.once;
            if ('passive' in options) {
                const passive = options.passive;
                this.passive = passive == null ? null : !!passive;
            }
            if ('signal' in options) {
                try {
                    this.signal = convertToAbortSignal(options.signal);
                } catch (e) {
                    if (Platform.current().isThrown(e)) {
                        e.message = messages.failedToReadProeperty('signal', 'AddEventListenerOptions', e.message);
                    }
                    throw e;
                }
            }
        }
    }
}

class EventListener {
    constructor(type, callback, options) {
        if (options !== Object(options)) {
            options = Object.create(null);
        }
        this.type = '' + type;
        this.callback = callback;
        this.capture = false;
        this.passive = null;
        this.once = false;
        this.signal = null;
        this.removed = false;
        for (const key of ['capture', 'passive', 'once', 'signal']) {
            if (key in options) {
                this[key] = options[key];
            }
        }
    }
}

/**
 * @param {Platform} platform
 */
export default function install(platform) {
    const realm = platform.getRealm();
    realm.EventListenerOptions = EventListenerOptions;
    realm.AddEventListenerOptions = AddEventListenerOptions;
    realm.EventListener = EventListener;
    realm.EventTarget = EventTarget;

    platform.realm.EventTarget = platform.createInterface(
        realm.EventTarget,
        { name: 'EventTarget', length: 0 },
        returnValueInterfaceInterceptor(
            constructorErrorMessageInterceptor(
                'EventTarget',
                requireNewTargetInterceptor(
                    validateConstructorTargetInterceptor(
                        () => Platform.current().getRealm().EventTarget,
                        () => {}
                    )
                )
            )
        )
    );

    Object.defineProperties(platform.realm.EventTarget.prototype, {
        addEventListener: {
            configurable: true,
            enumerable: true,
            writable: true,
            value: platform.createNativeFunction(
                { name: 'addEventListener', length: 2 },
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().EventTarget,
                    requireArgumentCountInterceptor(
                        2,
                        functionInterceptor(
                            function addEventListener(type, callback, options) {
                                const platform = Platform.current();
                                const realm = platform.getRealm();
                                options = new realm.AddEventListenerOptions(options);
                                const listener = new EventListener(type, callback, options);
                                this.addEventListener(listener);
                            }
                        )
                    )
                )
            )
        },
        removeEventListener: {
            configurable: true,
            enumerable: true,
            writable: true,
            value: platform.createNativeFunction(
                { name: 'removeEventListener', length: 2 },
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().EventTarget,
                    requireArgumentCountInterceptor(
                        2,
                        functionInterceptor(
                            function removeEventListener(type, callback, options) {
                                type = '' + type;
                                const platform = Platform.current();
                                const realm = platform.getRealm();
                                options = new realm.EventListenerOptions(options);
                                this.eventListenerList.filter(
                                    listener => listener.callback === callback && listener.type === type && listener.capture === options.capture
                                ).forEach(this.removeEventListener, this);
                            }
                        )
                    )
                )
            )
        },
        dispatchEvent: {
            configurable: true,
            enumerable: true,
            writable: true,
            value: platform.createNativeFunction(
                { name: 'dispatchEvent', length: 1 },
                validateThisImplementationInterceptor(
                    () => Platform.current().getRealm().EventTarget,
                    methodErrorMessageInterceptor(
                        'EventTarget',
                        'dispatchEvent',
                        requireArgumentCountInterceptor(
                            1,
                            requireArgumentTypeInterceptor(
                                0,
                                () => Platform.current().getRealm().Event,
                                functionInterceptor(
                                    function dispatchEvent(event) {
                                        const realm = Platform.current().getRealm();
                                        if (event.dispatching || !event.initialized) {
                                            platform.throw(realm.DOMException.create(messages.eventAlreadyDispatched()));
                                        }
                                        // TODO: Continue dispatchEvent method
                                    }
                                )
                            )
                        )
                    )
                )
            )
        }
    });
}
