from math import *

tot = 1020*1024*8

arr = list(range(2, tot+2))

print("seiving...")

i = 0
while i < tot:
    if i%(1024*32) == 0:
      print(i/(1024*32), "of", tot/1024/32)
      
    cur = arr[i]
    if cur == None:
      i += 1
      continue
      
    j = i + cur
    while j < tot:
      arr[j] = None
      j += cur

    i += 1

primes = [] 
for n in arr:
  if n != None:
    primes.append(n)

print("generating hash table sizes. . . ")

hashsizes = []
last = 1
for p in primes:
  if p/last > 1.7:
    hashsizes.append(p)
    last = p
    
print(hashsizes)
for h in hashsizes:
  for i in range(2, h-1):
    if float(h)/i == int(h/i):
      raise RuntimeError("nonprime!")