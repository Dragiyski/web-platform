import { fileURLToPath } from 'node:url';
import { dirname, resolve as resolvePath } from 'node:path';
import { promisify } from 'node:util';
import { EOL } from 'node:os';
import { spawn } from 'node:child_process';
import * as fs from 'node:fs';
import gyp from 'node-gyp';

const __file__ = fileURLToPath(import.meta.url);
const __dir__ = dirname(__file__);

const index = JSON.parse(fs.readFileSync(resolvePath(__dir__, 'index.json'), { encoding: 'utf-8' }));

const builder = gyp();
const commands = Object.create(null);
for (const name of Object.keys(builder.commands)) {
    commands[name] = builder.commands[name];
}

builder.parseArgv([...process.argv.slice(0, 2), 'configure', 'build', '--debug', '--loglevel=error']);
for (const command of builder.todo) {
    try {
        await commands[command.name].call(builder, command.args);
    } catch (e) {
        console.error(e);
        throw e;
    }
}

const nativePath = resolvePath(__dir__, '../build/Debug/native.node');
const testEnvironment = {
    ...process.env,
    JS_COMPILED_MODULE_PATH: dirname(nativePath)
};

process.stdout.write('TAP version 13\n');
if (!Array.isArray(index)) {
    throw new TypeError('The test index is not an array');
}
process.stdout.write(`1..${index.length}\n`);
for (let i = 0; i < index.length; ++i) {
    const test = index[i];
    if (test !== Object(test) || typeof test.file !== 'string' || typeof test.name !== 'string') {
        throw new TypeError(`Test ${i} is not an object containing properties "name" and "file"`);
    }
    const file = resolvePath(__dir__, test.file);
    if (file === __dir__ || !file.startsWith(__dir__)) {
        throw new TypeError('Test should be placed into test directory');
    }
    const result = await runTest(file, test);
    if (result.signal != null) {
        process.stdout.write(`not ok - ${test.name}\n# exit signal ${result.signal}\n`);
        const stderr = result.stderr.toString('utf-8').split('\n');
        for (const line of stderr) {
            process.stdout.write(`# ${line}\n`);
        }
    } else if (result.code === 0) {
        process.stdout.write(`ok - ${test.name}\n`);
    } else {
        process.stdout.write(`not ok - ${test.name}\n# exit code ${result.code}`);
        const stderr = result.stderr.toString('utf-8').split('\n');
        for (const line of stderr) {
            process.stdout.write(`# ${line}\n`);
        }
    }
}

const srcDir = resolvePath(__dir__, '../src');
const moduleDir = resolvePath(__dir__, '../build/Debug');
const coverageDir = resolvePath(moduleDir, 'coverage');

async function runTest(file) {
    const child = spawn(process.execPath, [file], {
        env: testEnvironment,
        stdio: 'pipe'
    });
    child.stdin.end();
    child.once('error', onError);
    child.once('exit', onExit);
    child.stdout.on('data', onStdoutData);
    child.stderr.on('data', onStderrData);
    let stdout = Buffer.allocUnsafe(0);
    let stderr = Buffer.allocUnsafe(0);
    const defer = {};
    defer.promise = new Promise((resolve, reject) => {
        defer.resolve = resolve;
        defer.reject = reject;
    });
    return defer.promise;

    function onError(error) {
        child.off('exit', onExit);
        defer.reject(error);
    }

    function onExit(code, signal) {
        defer.resolve({
            code,
            signal,
            stdout,
            stderr
        });
    }

    function onStdoutData(data) {
        stdout = Buffer.concat([stdout, data]);
    }

    function onStderrData(data) {
        stderr = Buffer.concat([stderr, data]);
    }
}
