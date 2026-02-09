# 🧪 실행 및 테스트 계획 (Execution Plan)

이 문서는 **Generator Game System**이 정상적으로 동작하는지 확인하기 위한 테스트 절차를 안내합니다.

---

## 1. 하드웨어 없이 테스트하기 (Serial Monitor 시뮬레이션)

ESP32 보드만 PC에 연결된 상태에서도 시리얼 통신을 통해 게임의 로직 흐름을 검증할 수 있습니다.

### 준비 단계
1.  **Arduino IDE**에서 `updated_generator.ino`를 엽니다.
2.  코드를 **업로드**합니다.
3.  **시리얼 모니터**를 엽니다 (단축키: `Ctrl + Shift + M`).
4.  **Baud Rate**를 `115200`으로 설정합니다.

### 상태 전이 테스트 시나리오
다음 명령어들을 순서대로 입력하여 각 단계가 정상적으로 변경되는지 확인합니다.

| 입력 명령어 | 예상 상태 (Serial Output) | LED 반응 (예상) | 비고 |
| :--- | :--- | :--- | :--- |
| (부팅 직후) | `STATE_BOOT` | - | 초기 상태 |
| `S` | `STATE: SETTING` | ⚪ White | 초기 설정 모드 |
| `R` | `STATE: READY` | 🔴 Red | 게임 준비 완료 |
| `A` | `STATE: ACTIVATE` | 🟡 Yellow | 게임 시작 (로그인 성공) |
| `msf` | `STATE: STARTER_FINISH` | 🟢 Green | 발전 완료 (게이지 Full) |
| `mr` | `STATE: REPAIRED` | 🔵 Blue | 수리 완료 (모터 회전) |
| `mra` | `STATE: REPAIRED_ALL` | 🔵 Blue | 탈출 성공 (모터 정지) |

---

## 2. 실제 하드웨어 포함 테스트 (Full Hardware Test)

모든 부품(LED, 모터, 디스플레이, 엔코더, RFID)이 연결된 상태에서의 테스트 절차입니다.

### 하드웨어 준비
1.  모든 부품이 회로도대로 정확히 연결되었는지 확인합니다.
2.  12V/5V 전원을 공급합니다.

### 게임 플레이 시나리오 테스트
1.  **대기 상태**: 전원을 켜면 흰색 LED(`Setting`) 또는 빨간색 LED(`Ready`)가 들어와야 합니다.
2.  **로그인**: 등록된 RFID 카드를 태그합니다. 노란색 LED가 켜지며 `Activate` 상태가 되어야 합니다.
3.  **발전**: 로터리 엔코더 핸들을 돌립니다.
    *   LED 게이지가 서서히 차오르는지 확인합니다.
    *   모터가 회전 속도에 맞춰 진동(햅틱)을 주는지 확인합니다.
4.  **발전 완료**: 게이지를 끝까지 채우면 자동으로 초록색 LED(`Starter Finish`)로 바뀌어야 합니다.
5.  **수리 완료**: 게임 시나리오에 따라 파란색 LED(`Repaired`)가 켜지고 모터가 힘차게 돌아야 합니다.


---

## 4. 서버 통신 및 데이터 검증 (Server & Data Verification)

ESP32가 서버와 데이터를 잘 주고받고 있는지 확인하는 방법입니다.

### 📡 시리얼 로그 해석
시리얼 모니터(`115200` baud)에 출력되는 로그를 통해 통신 상태를 파악할 수 있습니다.

#### 1. RFID 태그 인식 및 서버 요청
카드를 태그하면 다음과 같은 로그가 나타납니다.
```text
TAG DETECTED            <-- 태그가 하드웨어적으로 인식됨
Tag User: GxPx          <-- 태그된 카드의 ID (예: G1P1)
Player Tagged           <-- 서버로부터 역할을 받아옴 (Player/Tagger/Ghost)
```
*   **성공**: `Tag User: ...` 뒤에 문자가 깨지지 않고 나오고, `Player Tagged` 등이 출력되면 통신 성공.
*   **실패**: 아무 반응이 없거나 `PN532 FAIL` 메시지가 뜨면 하드웨어 연결 확인 필요.

#### 2. 배터리 충전 및 상태 업데이트
배터리 충전 시 다음과 같은 로그가 발생합니다.
```text
BatteryPackCharge Executing...
Charging...
```
*   이때 내부적으로 `has2wifi.Send(...)` 함수가 호출되어 서버로 `battery_pack` 수치를 전송합니다.
*   서버 데이터가 변경되면 `Wifi.ino`의 `DataChanged()` 함수가 호출되며, 관련 상태 로그가 찍힐 수 있습니다.

#### 3. 상태 변경 명령 수신
서버(또는 웹)에서 게임 상태를 변경하면 `Wifi.ino`가 이를 감지하고 FSM에 명령을 내립니다.
```text
FSM Command: S          <-- 'setting' 상태 수신
STATE: SETTING          <-- FSM이 SETTING 상태로 전환됨
```
*   웹 페이지나 관리자 도구에서 상태를 바꿨을 때 위 로그가 찍히는지 확인하세요.

---

## 5. 문제 해결 (Troubleshooting)


*   **컴파일 오류**: 라이브러리가 모두 설치되었는지 확인하세요 (`Adafruit NeoPixel`, `Adafruit PN532` 등).
*   **시리얼 반응 없음**: Baud Rate가 `115200`인지 확인하세요.
*   **상태가 안 바뀜**: `GameFSM.cpp`의 `executeCommand` 함수 로그를 확인하세요.
