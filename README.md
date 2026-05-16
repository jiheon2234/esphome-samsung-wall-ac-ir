# ESPHome Samsung Wall AC IR

- 삼성 벽걸이 에어컨용 ESPHome 외부 컴포넌트입니다.
- IR 리모컨을 Home Assistant의 `climate` 엔티티로 제어할 수 있게 합니다.
- 삼성 에어컨 `AR06M1130HZ`의 IR 패킷을 기준으로 작성되었습니다.
- 패킷 계산식은 아직 찾지 못해 lookup table을 사용했습니다.

## 지원 기능

현재는 아래 기능만 지원합니다.

- 전원 OFF
- 냉방 모드
- 팬 1단계 기준 패킷
- 온도 16~26도

## 추후 지원 예정

- 난방, 제습, 송풍 등의 모드
- 팬 속도 제어
- 실제 에어컨 리모컨을 사용했을 때 Home Assistant의 `climate` 상태도 같이 갱신하는 기능

## 기록한 것들

팬/모드 등의 상태가 고정된 상태에서 캡처한 데이터이므로 정확하지 않을 수 있습니다.

- [정리](notes.md)
- [캡처한 데이터](capture_state.txt)
