var fs = require("fs");
var Jimp = require("jimp");
var Path = require("path");

var images = [];
files = fs.readdirSync("./in");
files.forEach(file => {
    if(file.match(/(\.jpg)|(\.png)|(\.bmp)$/)){
        console.log("found: "+ file);
        images.push(file);
    }
});
var processes = [];

images.forEach(async imgfile => {
    processes.push(processImage(imgfile));
});


Promise.all(processes).then(imgdata => {
    
    var imglengths = imgdata.map(i=>i.pixelcolors.length);
    var pixeldata = imgdata.map(i=>i.pixelcolors);
    code = "";
    code += "const uint16_t imglengths["+imglengths.length+"] =" +arrs_c(imglengths.map(l => l/72))+";\n\n";
    code += "const uint32_t palette["+palette.length+"] PROGMEM =" +arrs_c(palette)+";\n\n";
    
    imgdata.forEach((img,c) => {
        code += "const uint16_t img"+c+"["+img.pixelcolors.length+"] PROGMEM =  ";
        code += arrs_c(img.pixelcolors)+";\n\n";
    });
    code += "const uint16_t *const img_table[] PROGMEM =  { img0";
    for(var i=1;i<imgdata.length;i++){
        code += ", img"+i+"";
    }      
    code += "};\n\n";
      
    



    code += "const char* myhtml ICACHE_RODATA_ATTR = \"";
    html = "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no' />";
    html += `<style>
    html,
    body {
      background: black;
      font-size: 12px;
      color: white;
      text-align: center;
    }
    * {
      font-family: sans-serif;
    }
    .poilink {
      line-height: 70px;
      text-align: center;
      display: block;
      float: left;
      overflow: hidden;
      height: 70px;
      width: 20%;
      text-decoration: none;
      border: solid black 2px;
      box-sizing: border-box;
      transition: all 0.3s ease;
      transform-origin: bottom;
      background: whitesmoke;
      color:black;

    }
    .wide {
      width: 100%;
    }
    .active {
        background: cadetblue;
        color: whitesmoke;
        transform: scale(.95);
    }
    img {
      position: absolute;
    }
    h1{
        margin:5px;
    }
    h2{
        color: indianred;
        margin:5px;
    }
  </style>
  `;
  html += "<h1>* BitpixelPoi *</h1>";
  html += "<h2>- pattern controller -</h2>";
  html += "<div class='poilink wide' data-json='{off:1}'>OFF</div>";
    html += "<div class='poilink wide' data-json='{standby:1}'>STANDBY</div>";
    var bigjson = {
      seq: []
    };
    imgdata.forEach((img, c) => {
      bigjson["seq"].push({
        id: c,
        r_t: 1500
      });
    });

    html +=
      "<div class='poilink wide' data-json=\\\"" +
      JSON.stringify(bigjson).replace(/"/g, "'") +
      '\\">ALL 1,5 Sec</div>';

    bigjson = {
      seq: []
    };
    imgdata.forEach((img, c) => {
      bigjson["seq"].push({
        id: c,
        r_t: 3500
      });
    });

    html +=
      "<div class='poilink wide' data-json=\\\"" +
      JSON.stringify(bigjson).replace(/"/g, "'") +
      '\\">ALL 3,5 Sec</div>';

    bigjson = {
      seq: []
    };
    imgdata.forEach((img, c) => {
      bigjson["seq"].push({
        id: c,
        r_t: 5000
      });
    });

    html +=
      "<div class='poilink wide' data-json=\\\"" +
      JSON.stringify(bigjson).replace(/"/g, "'") +
      '\\">ALL 5 Sec</div>';
    
      
    html += "<div class='poilink' data-json='{spark:10}'>Fire10</div>";
    html += "<div class='poilink' data-json='{spark:50}'>Fire50</div>";
    html += "<div class='poilink' data-json='{spark:100}'>Fire100</div>";
    imgdata.forEach((img,c) => {
        html += "<div class='poilink' data-json='{img:"+c+"}'><span>"+img.filename+"</span></div>";
    });

    html += "<div class='poilink' data-json='{pattern:1}'>DEMO PATTERNS</div>";
    
    var js = `

    var poilinks = document.getElementsByClassName('poilink');
    for (var i = 0; i < poilinks.length; i++) {
        poilinks[i].addEventListener('click',handleclick);
    }
    


    function handleclick(e){
        var data = this.dataset.json;
        console.log(data);
        setPoilinkActive(this);
        socket.send(data);
        return false;
    }
    
    var socket;
    function connect(){
        socket = new WebSocket('ws://42.42.42.42:81');
        socket.onopen = function(e) {
            alert('Connected');
        };
        
        socket.onmessage = function(event) {
            console.log('[message] Data received from server: '+event.data);
        };
        
        socket.onclose = function(event) {
            if(confirm('Try again?')){
                connect();
            }
        };
        socket.onerror = function(err) {
            socket.close();
        };
    }
    function setPoilinkActive(elem){
        for (var i = 0; i < poilinks.length; i++) {
            poilinks[i].classList.remove('active');
        }        
        elem.classList.add('active');
    }

    connect();
  `;
    
  html += "<script>"+js+"</script>";
    


  html = html.replace(/\n/g,"").replace(/\r/g,"");
    fs.writeFile("out.html", html, function(err) {
        if(err) {
            return console.log(err);
        }

        console.log("The file was saved!");
    })

    code += html+"\";";
    fs.writeFile("mypixels.h", code, function(err) {
        if(err) {
            return console.log(err);
        }

        console.log("The file was saved!");
    })


    }
); 

function arrs_c(arr){
    return  JSON.stringify(arr).replace(/\[/g,'{').replace(/\]/g,'}');
}
const  gamma8 = [
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 ];

var palette = [];

function processImage(file){
    return Jimp.read("in/"+file).then(async image => {
        console.log("processImage: "+ file);
        var pixelcolors = [];
        image.resize(Jimp.AUTO,72, Jimp.RESIZE_BICUBIC);
        
        var oimage = await new Jimp(image.bitmap.width, image.bitmap.height);

        // console.log(image.bitmap.width);
        // console.log(image.bitmap.height);
        for(var x=0;x<image.bitmap.width;x++){
            // pixelcolors[x] = [];
            for(var y=0;y<image.bitmap.height;y++){
                
                let pcol = image.getPixelColor(x, y);
                let rgbcol = Jimp.intToRGBA(pcol);

                rgbcol.r = gamma8[rgbcol.r];
                rgbcol.g = gamma8[rgbcol.g];
                rgbcol.b = gamma8[rgbcol.b];

                roundColor(rgbcol,8);

                pcol = Jimp.rgbaToInt(rgbcol.r,rgbcol.g,rgbcol.b,rgbcol.a);

                let pcolcorrect = Math.floor(pcol/16/16); //remove alpha (?)

                var colindex = palette.indexOf(pcolcorrect);
                if(colindex===-1){
                    palette.push(pcolcorrect);
                    colindex = palette.indexOf(pcolcorrect);
                }

                oimage.setPixelColor(pcol, x, y);

                pixelcolors.push(colindex);
            }
        }
        oimage.write("out/"+file);
        
        // console.log("pixelcolors: "+ pixelcolors.length);
        return image.scaleToFit(16,16).getBase64Async("image/jpeg")
            .then(preview => { return {pixelcolors:pixelcolors, preview:preview, filename: Path.basename(file,Path.extname(file))};} )
                .then(data => { data.avgcolor = "#"+Math.floor(image.clone().resize(1,1).getPixelColor(1,1)/16/16).toString(16); return data;})
    });

    
    
}

function roundColor(rgbcol, i){
    if(i<7){ throw new Exception("must be over 6 to guarantee 16 bit for index")}
    rgbcol.r = Math.floor(rgbcol.r/i)*i;
    rgbcol.g = Math.floor(rgbcol.g/i)*i;
    rgbcol.b = Math.floor(rgbcol.b/i)*i;
}
