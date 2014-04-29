(function() {
    if (typeof process === 'undefined') {
        var initialized = false;

        onmessage = function(event) {
            if (!initialized) {
                Module.ccall('init', 'number', [], []);
                initialized = true;
            }
            if (typeof event.data === 'object') {
                if (event.data.book) {
                    var book = event.data.book;
                    var byteArray = new Uint8Array(book);
                    var buf = Module._malloc(book.byteLength);
                    Module.HEAPU8.set(byteArray, buf);
                    Module.ccall('set_book', 'number', ['number', 'number'], [buf, book.byteLength]);
                }
            }
            else {
                Module.ccall('uci_command', 'number', ['string'], [event.data]);
            }
        };

        console = {
            log: function(line) {
                postMessage(line);
            }
        };
    }
    else {
        process.stdin.resume();
        var lines = null;
        var init = function() {
            if (lines === null) {
                Module.ccall('init', 'number', [], []);
                lines = '';
            }
        };
        process.nextTick(init);
        process.stdin.on('data', function(chunk) {
            init();
            lines += chunk;
            var match;
            while (match === lines.match(/\r\n|\n\r|\n|\r/)) {
                var line = lines.slice(0, match.index);
                lines = lines.slice(match.index + match[0].length);
                Module.ccall('uci_command', 'number', ['string'], [line]);
                if (line === 'quit') {
                    process.exit();
                }
            }
        });
        process.stdin.on('end', function() {
            process.exit();
        });

    }
})();
