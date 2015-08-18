
var buf = new ArrayBuffer(16);

var fview = new Float32Array(buf);
var dview = new Float64Array(buf);
var iview = new Int32Array(buf);

function cbrt(n) {
  dview[0] = n;
  //iview[0] = iview[0]/3 + 709921077; //float
  iview[1] = iview[1]/3 + 715094163; //double
  
  f = dview[0]; //double
  f = (2.0*f*f*f + n) / (3.0*f*f);
  f = (2.0*f*f*f + n) / (3.0*f*f);
  f = (2.0*f*f*f + n) / (3.0*f*f);
  f = (2.0*f*f*f + n) / (3.0*f*f);

  return f;
}

function exact_cbrt(n) {
  fview[0] = n;
  iview[0] = iview[0]/3 + 709921077;
  
  f = fview[0];
  for (var i=0; i<32; i++) {
    f = (2.0*f*f*f + n) / (3.0*f*f);
  }

  return f;
}

for (var i=0; i<256; i++) {
  var err = cbrt(i)-exact_cbrt(i);
  if (err == 0.0) continue;
  console.log(cbrt(i)-exact_cbrt(i));
}