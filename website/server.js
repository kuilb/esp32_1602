const express = require('express');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 12222;

const multer = require('multer');
const upload = multer({ dest: 'uploads/' }); // 上传文件保存到 uploads 目录

// 解析 JSON / 表单
app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// 静态文件：让浏览器能访问你的 HTML / JS
const webRoot = path.join(__dirname, 'site');

// 自定义静态文件中间件，强制为无扩展名的文件设置 Content-Type
app.use((req, res, next) => {
    const url = req.path;
    // 如果是 /citysearch 这种无扩展名的路径，且对应文件存在
    if (!path.extname(url)) {
        const filePath = path.join(webRoot, url);
        if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) {
            res.type('html');
            return res.sendFile(filePath);
        }
    }
    next();
});

app.use('/', express.static(webRoot));

console.log('webRoot:', webRoot);
console.log('__dirname:', __dirname);
console.log('process.cwd():', process.cwd());
console.log('index file exists:', fs.existsSync(path.join(webRoot, 'index.html')));

app.listen(PORT, () => {
    console.log(`测试服务器已启动: http://localhost:${PORT}`);
});

app.get('/wifi',(req, res) => {
    console.log('access /wifi');
});

app.post('/wifi_set',(req, res) => {
    console.log('access /wifi_set');
    const {ssid, password} = req.body;
    console.log('ssid:', ssid);
    console.log('password:', password);
    res.status(200).json({ 
        success: true 
    });
});

let inScan = false;
let testCount = 0;
app.get('/wifi_scan',(req, res) => {
    console.log('access /wifi_scan');
    if(!inScan){
        inScan = true;
        res.status(202).json({
            status: 'scanning'
        })
        testCount += 1;
    }
    else{
        if(testCount < 2){
            res.status(202).json({
            status: 'scanning'
            })
            testCount += 1;
        }
        else{
            inScan = false;
            testCount = 0;
            res.status(200).json({
                status: 'done',
                networks: [
                    { ssid: 'Network_1', rssi: -40, secure: true },
                    { ssid: 'Network_2', rssi: -70, secure: false },
                    { ssid: 'Network_3', rssi: -60, secure: true },
                    { ssid: 'Network_4', rssi: -80, secure: true },
                    { ssid: 'Network_5', rssi: -40, secure: true },
                    { ssid: 'Network_6', rssi: -70, secure: false },
                    { ssid: 'Network_7', rssi: -60, secure: true },
                    { ssid: 'Network_8', rssi: -80, secure: true },
                    { ssid: 'Network_9', rssi: -40, secure: true },
                    { ssid: 'Network_10', rssi: -70, secure: false },
                    { ssid: 'Network_11', rssi: -60, secure: true }
                ]
            });
        }
    }
});

app.get('/index.html', (req, res) => {
    console.log('access /');
});

const otaStats = {
    idle: 0,
    running: 1,
    success: 2,
    failed: 3
};
let currentOtaStats = otaStats.idle;

let otaProgress = 0;
let inProgress = false;
app.post('/ota/progress', (req, res) => {
    res.status(405).json({ 
        error: '请使用 GET 方法获取进度' 
    });
});
app.get('/ota/progress', (req, res) => {
    console.log('access /ota/progress');
    if(currentOtaStats == otaStats.idle){
        res.status(200).json({ 
            progress:otaProgress,
            status: 'idle',
        });
    }
    if(currentOtaStats == otaStats.running){
        otaProgress += 10;
        if(otaProgress >= 100){
            otaProgress = 100;
            currentOtaStats = otaStats.success;
            res.status(200).json({ 
                progress:otaProgress,
                status: 'success',
            });
            otaProgress = 0;
            inProgress = false;
            return;
        }
        res.status(200).json({ 
            progress:otaProgress,
            status: 'in_progress',
        });
    }
});

// /ota/upload: 固件上传接口
app.get('/ota/upload', (req, res) => {
    console.log('get /ota/upload GET');
    res.status(405).json({ 
        error: '请使用 POST 方法上传固件' 
    });
});
app.post('/ota/upload', upload.single('firmware'), (req, res) => {
    console.log('post /ota/upload'); 
    if(!req.file){
        res.status(400).json({ 
            error: '未收到文件' 
        });
        return;
    }

    console.log('收到文件:', req.file);
    res.status(200).json({ 
        success: 'true', 
    });
});

// /ota/url: 固件下载地址接口
app.get('/ota/url', (req, res) => {
    console.log('access /ota/url');
    if(inProgress){
        res.status(409).json({
            error: '已有固件更新正在进行中'
        });
        return;
    }
    if(!req.query.url){
        res.status(400).json({ 
            error: '缺少 url 参数' 
        });
        return;
    }
    inProgress = true;
    console.log('get url: ' + req.query.url);
    res.status(202).json({ 
        statusURL: 'http://localhost:12222/ota/progress', 
    });
    currentOtaStats = otaStats.running;
});

var pollCount = 0;
app.get('/citysearch_result', (req, res) => {
    console.log('get /citysearch_result');
    const location = req.query.location;
    console.log('search location:', location);
    if(pollCount < 1){
        pollCount++;
         res.status(202).json({
            status: 'processing'
        });
    }
    else{
        console.log('return search result for location:', location);
        pollCount = 0;
        res.status(200).json({
            success: true,
            results:[ 
                   {
                    name: '北京',
                    adm1: '北京市',
                    country: '中国',
                    locid: '101010100',
                    fxlink: 'https://www.qweather.com/weather/beijing-101010100.html'
                },
                {
                    name: '上海',
                    adm1: '上海市',
                    country: '中国',
                    locid: '101020100',
                    fxlink: 'https://www.qweather.com/weather/shanghai-101020100.html'
                },
                {
                    name: '东京',
                    adm1: '东京都',
                    country: '日本',
                    locid: '1850147',
                    fxlink: 'https://www.qweather.com/weather/tokyo-1850147.html'
                },
                {
                    name: '纽约',
                    adm1: '纽约州',
                    country: '美国',
                    locid: '5128581',
                    fxlink: 'https://www.qweather.com/weather/new-york-5128581.html'
                },
                {
                    name: '伦敦',
                    adm1: '英格兰',
                    country: '英国',
                    locid: '2643743',
                    fxlink: 'https://www.qweather.com/weather/london-2643743.html'
                },
                {
                    name: '巴黎',
                    adm1: '法兰西岛大区',
                    country: '法国',
                    locid: '2988507',
                    fxlink: 'https://www.qweather.com/weather/paris-2988507.html'
                }
        ]
        });
    }
   
});

app.post('/set_location', (req, res) => {  
    console.log('post /set_location body:\n' + JSON.stringify(req.body))
    const {locid} = req.body;
    console.log('set location to:', locid);
    res.status(200).json({ 
        ok: true,
        locid: locid
    });
});

// /set: 密钥保存接口
app.post('/set', (req, res) => {
    function isValidUrl(str) {
        try {
            new URL(str);
            return true;
        } catch (e) {
            return false;
        }
    }
    
    function isValidID(str) {
        if(str.length != 10)    return false;
        if(!/^[a-zA-Z0-9]+$/.test(str)) return false;
        return true;
    }

    function isValidEd25519PrivateKey(str) {
        if(str.length != 64)    return false;
        if(!/^[a-fA-F0-9]+$/.test(str)) return false;
        try {
            buf = Buffer.from(str, 'base64');
        } catch {
            return false;
        }
        // Ed25519 PKCS#8 DER 通常 48字节，开头 0x30
        return buf.length === 48 && buf[0] === 0x30;
    }

    console.log('post /set body:\n' + JSON.stringify(req.body))

    const {apiHost, kID, projectID, privateKey} = req.body;
    console.log(apiHost + '\n' + kID + '\n' + projectID + '\n' + privateKey);

    if(!apiHost || !kID || !projectID || !privateKey){
        res.status(400).json({ ok: false, error: '有参数缺失' });
        return;
    }
    else if(apiHost == '' || kID == '' || projectID == '' || privateKey == ''){
        res.status(400).json({ ok: false, error: '有参数为空' });
        return;
    }
    else if(!isValidUrl(apiHost)){
        res.status(400).json({ ok: false, error: '无效的 apiHost URL' });
        return;
    }
    else if(!isValidID(kID)){
        res.status(400).json({ ok: false, error: '无效的 kID' });
        return;
    }
    else if(!isValidID(projectID)){
        res.status(400).json({ ok: false, error: '无效的 projectID' });
        return;
    }
    else if(!isValidEd25519PrivateKey(privateKey)){
        res.status(400).json({ ok: false, error: '无效的 privateKey' });
        return;
    }

    res.json({ ok: true });
});

app.get('/get_api_info', (req, res) => {
    var apiHost = 'test.com';
    var kID = 'test_kid';
    var projectID = 'test_project';
    var privateKey = 'test_key';
    res.json({ apiHost: apiHost, kID: kID, projectID: projectID, privateKey: privateKey });
});

// /exit: 退出接口
app.get('/exit', (req, res) => {
    console.log('/exit send OK');
    res.status(200).send('OK');
});