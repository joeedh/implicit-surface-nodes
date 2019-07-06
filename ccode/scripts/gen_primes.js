"use strict";
var tot = 1020*1024*256;

var arr = new Int32Array(tot);
for (var i=0; i<tot; i++) {
  arr[i] = i+2;
}

console.log("seiving...");

var i = 0;
while (i < arr.length) {
    if (i % (1024*32) == 0) {
      console.log(i/(1024*32), "of", tot/1024/32);
    }
    
    var cur = arr[i];
    if (cur == -1) {
      i += 1
      continue
    }
    
    var j = i + cur;
    while (j < arr.length) {
      arr[j] = -1;
      j += cur
    }
    
    i++;
}

var primes = [];
for (var i=0; i<arr.length; i++) {
  if (arr[i] > 0) {
    primes.push(arr[i]);
  }
}

console.log("generating hash table sizes. . . ");

var hashsizes = []
var last = 1
for (var i=0; i<primes.length; i++) {
  var p = primes[i];
  if (p/last > 1.7) {
    hashsizes.push(p)
    last = p
  }
}

console.log(hashsizes);
/*
for h in hashsizes:
  for i in range(2, h-1):
    if float(h)/i == int(h/i):
      raise RuntimeError("nonprime!")
      
*/