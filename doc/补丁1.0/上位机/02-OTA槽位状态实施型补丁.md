# 上位机 OTA 槽位状态实施型补丁

## 1. 实施目标

在上位机仓库中增加对 `0x08/0x09/0x0A` 三个槽位状态的识别、显示和测试，同时保持原有 OTA 流程、帧协议和最终成功判定不变。

实施基线：

- 仓库：`PY-MyBootLoader-OTA`
- 分支：`main`
- 基线提交：`070abf7`
- 协议版本：`OTA-PROTO-1.0.1`

## 2. 修改文件清单

| 文件 | 修改类型 | 说明 |
| --- | --- | --- |
| `ota_host/protocol.py` | 修改 | 增加三个运行状态常量映射 |
| `web/app.js` | 修改 | 增加中文映射和颜色判定 |
| `web/index.html` | 修改 | 增加升级后手动复位和再次查询提示 |
| `tests/test_protocol.py` | 修改 | 增加新状态解析测试 |
| `tests/test_engine.py` | 修改 | 增加查询新状态并更新设备状态的测试 |
| `doc/上位机/01-通讯协议完整规范.md` | 修改 | 更新协议版本、状态表、生命周期说明 |
| `doc/上位机/02-通讯协议简明说明.md` | 修改 | 同步状态表与用户流程 |
| `doc/上位机/03-对接与测试清单.md` | 修改 | 增加新状态联调项 |
| `docs/UI-PLAN.md` | 修改 | 增加新状态颜色与提示说明 |
| `README.md` | 修改 | 更新协议版本与升级后操作说明 |

> 设备侧文档副本若与上位机仓库中的 `doc/设备/*` 保持同步，也应一次性同步为相同状态表，避免两份规范漂移。

## 3. 协议层修改

### 3.1 `ota_host/protocol.py`

在 `RUN_STATUS` 中追加：

```python
0x08: "RUN_PENDING_CONFIRM",
0x09: "RUN_CONFIRMED_OK",
0x0A: "RUN_CONFIRMED_BAD",
```

在 `RUN_STATUS_ZH` 中追加：

```python
0x08: "运行待确认",
0x09: "确认运行正常",
0x0A: "确认无法运行",
```

要求：

- 不删除、不重排现有 `0x00~0x07`。
- `parse_status()` 继续返回 `run_status_name` 和 `run_status_zh`。
- 未知状态仍使用 `RESERVED/保留状态`。

### 3.2 版本注释

将协议相关文档和模块说明统一改为 `OTA-PROTO-1.0.1`。若需要修改 `protocol.py` 模块文档字符串，可改为：

```python
"""OTA-PROTO-1.0.1 frame, CRC and control-page primitives."""
```

本次不修改任何帧字段长度、CRC 算法或状态记录四字节顺序 `[Run_status][Page][Block][Error]`。

## 4. Web 界面修改

### 4.1 中文状态映射

在 `web/app.js` 的 `STATUS_ZH` 中追加：

```javascript
RUN_PENDING_CONFIRM: "运行待确认",
RUN_CONFIRMED_OK: "确认运行正常",
RUN_CONFIRMED_BAD: "确认无法运行",
```

### 4.2 颜色判定

将 `renderDevice()` 中现有颜色判断扩展为：

```javascript
if (device.run_status === 0x05 ||
    device.run_status === 0x06 ||
    device.run_status === 0x09) {
  statusNode.classList.add("ok");
} else if (device.run_status === 0x03 ||
           device.run_status === 0x04 ||
           device.run_status === 0x08) {
  statusNode.classList.add("warn");
} else if (device.run_status === 0x07 ||
           device.run_status === 0x0A) {
  statusNode.classList.add("bad");
}
```

语义：

- `0x05`：有效备用，等待下一次重启。
- `0x06`：一号槽正常运行。
- `0x09`：对应缓存槽确认正常。
- `0x08`：试运行，等待自检，必须使用警告色。
- `0x0A`：明确坏版本，必须使用错误色。
- `0x07`：OTA/Flash 错误，保持错误色。

### 4.3 用户提示

在升级说明区域增加手动复位提示。现有提示“升级过程中请勿断开串口或设备电源”之后追加：

```html
<p>升级完成后设备状态应为“有效备用”。请手动复位设备以触发 BootLoader 切换；
重新连接后查询状态：若为“运行待确认”，等待 APP 自检结束再查询；
若为“确认无法运行”，设备将尝试回滚或进入安全模式。</p>
```

不要增加自动重启按钮。

## 5. 测试实施

### 5.1 `tests/test_protocol.py`

增加表驱动测试：

```python
def test_extended_run_status_names(self):
    expected = {
        0x08: ("RUN_PENDING_CONFIRM", "运行待确认"),
        0x09: ("RUN_CONFIRMED_OK", "确认运行正常"),
        0x0A: ("RUN_CONFIRMED_BAD", "确认无法运行"),
    }
    for code, (name, zh) in expected.items():
        parsed = parse_status(status_record(code, 0xFF, 0xFF, 0x00))
        self.assertEqual(parsed["run_status"], code)
        self.assertEqual(parsed["run_status_name"], name)
        self.assertEqual(parsed["run_status_zh"], zh)
```

### 5.2 `tests/test_engine.py`

增加一次 `GetSta` 查询测试，要求：

1. 假设备返回 `SetSta(0x08, 0xFF, 0xFF, 0x00)`；
2. 事务正常结束；
3. `engine.snapshot()["device"]` 中状态码、英文名和中文名分别为 `0x08`、`RUN_PENDING_CONFIRM`、`运行待确认`；
4. 同样覆盖 `0x09` 和 `0x0A`。

### 5.3 保持原有成功判定测试

现有测试必须继续验证：

- OTA 最终 `SetSta/SetEnd` 只有返回 `0x05` 才成功；
- 若返回 `0x08/0x09/0x0A`，不得通过 `_perform_final_handshake()` 的成功条件；
- 页 CRC 不一致仍失败。

## 6. 实施步骤

1. 更新 `protocol.py` 两个状态映射表。
2. 更新 `web/app.js` 中文映射与 `renderDevice()` 颜色逻辑。
3. 更新 `web/index.html` 手动复位提示。
4. 添加协议层和状态机测试。
5. 同步更新两份协议规范、简明说明、测试清单、UI 规划和 README。
6. 运行完整测试。
7. 人工启动上位机，用假设备或真实设备查询三种状态并检查颜色、中文和原始十六进制值。

## 7. 验证命令

```powershell
Set-Location C:\Users\aguoz\Desktop\手柄\PY-MyBootLoader-OTA
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
```

若虚拟环境尚未安装依赖：

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

静态检查建议：

```powershell
rg -n "RUN_PENDING_CONFIRM|RUN_CONFIRMED_OK|RUN_CONFIRMED_BAD" .
rg -n "0x08|0x09|0x0A|运行待确认|确认运行正常|确认无法运行" doc docs web ota_host tests README.md
```

## 8. 验收标准

- 状态查询收到 `0x08` 时显示“运行待确认”并使用警告色。
- 状态查询收到 `0x09` 时显示“确认运行正常”并使用成功色。
- 状态查询收到 `0x0A` 时显示“确认无法运行”并使用错误色。
- 未识别状态仍显示“保留状态”，不会导致异常。
- OTA 成功仍在最终收到 `0x05` 且 CRC 全部一致时才成立。
- 所有现有测试和新增测试通过。
- 不新增协议命令，不自动复位设备。