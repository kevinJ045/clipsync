#!/usr/bin/env node
const http = require('http');
const { spawn } = require('child_process');

spawn('wl-paste', ['--watch sh -c "curl -X POST http://192.168.0.109:52913 -d \"\$(wl-paste)\""']);


const server = http.createServer((req, res) => {
	let body = '';
	req.on('data', (chunk) => {
		body += chunk.toString();
	});

	req.on('end', () => {
		const wlCopy = spawn('wl-copy');

		wlCopy.stdin.write(body);
		wlCopy.stdin.end();

		res.statusCode = 200;
		res.end('done');
	});
});

server.listen(52913);
