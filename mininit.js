function g(a){throw a;}var l=void 0,n=!0,p=null,q=!1;function r(){return function(){}}function s(a){return function(){return a}}var t;
if("undefined"===typeof process){var aa=q;onmessage=function(a){aa||(v.ccall("init","number",[],[]),aa=n);if("object"===typeof a.data){if(a.data.book){var a=a.data.book,b=new Uint8Array(a),c=v._malloc(a.byteLength);v.HEAPU8.set(b,c);v.ccall("set_book","number",["number","number"],[c,a.byteLength])}}else v.ccall("uci_command","number",["string"],[a.data])};console={log:function(a){postMessage(a)}}}else{process.Fc.Ag();var ba=p,ca=function(){ba===p&&(v.ccall("init","number",[],[]),ba="")};process.sg(ca);process.Fc.on("data",
function(a){ca();for(ba+=a;m===ba.match(/\r\n|\n\r|\n|\r/);)a=ba.slice(0,l.index),ba=ba.slice(l.index+l[0].length),v.ccall("uci_command","number",["string"],[a]),"quit"===a&&process.gc()});process.Fc.on("end",function(){process.gc()})}var v;v||(v=eval("(function() { try { return Module || {} } catch(e) { return {} } })()"));var da={},ea;for(ea in v)v.hasOwnProperty(ea)&&(da[ea]=v[ea]);
var fa="object"===typeof process&&"function"===typeof require,ga="object"===typeof window,ha="function"===typeof importScripts,ia=!ga&&!fa&&!ha;
if(fa){v.print||(v.print=function(a){process.stdout.write(a+"\n")});v.printErr||(v.printErr=function(a){process.stderr.write(a+"\n")});var ja=require("fs"),ka=require("path");v.read=function(a,b){var a=ka.normalize(a),c=ja.readFileSync(a);!c&&a!=ka.resolve(a)&&(a=path.join(__dirname,"..","src",a),c=ja.readFileSync(a));c&&!b&&(c=c.toString());return c};v.readBinary=function(a){return v.read(a,n)};v.load=function(a){la(read(a))};v.arguments=process.argv.slice(2);module.exports=v}else ia?(v.print||(v.print=
print),"undefined"!=typeof printErr&&(v.printErr=printErr),v.read="undefined"!=typeof read?read:function(){g("no read() available (jsc?)")},v.readBinary=function(a){return read(a,"binary")},"undefined"!=typeof scriptArgs?v.arguments=scriptArgs:"undefined"!=typeof arguments&&(v.arguments=arguments),this.Module=v,eval("if (typeof gc === 'function' && gc.toString().indexOf('[native code]') > 0) var gc = undefined")):ga||ha?(v.read=function(a){var b=new XMLHttpRequest;b.open("GET",a,q);b.send(p);return b.responseText},
"undefined"!=typeof arguments&&(v.arguments=arguments),"undefined"!==typeof console?(v.print||(v.print=function(a){console.log(a)}),v.printErr||(v.printErr=function(a){console.log(a)})):v.print||(v.print=r()),ga?this.Module=v:v.load=importScripts):g("Unknown runtime environment. Where are we?");function la(a){eval.call(p,a)}"undefined"==!v.load&&v.read&&(v.load=function(a){la(v.read(a))});v.print||(v.print=r());v.printErr||(v.printErr=v.print);v.arguments||(v.arguments=[]);v.print=v.print;v.ta=v.printErr;
v.preRun=[];v.postRun=[];for(ea in da)da.hasOwnProperty(ea)&&(v[ea]=da[ea]);