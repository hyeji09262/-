# GLFW 빌드 및 검증

## 빌드

Visual Studio 2022에서 **C++를 사용한 데스크톱 개발**, MSVC v143, Windows SDK를 설치한다.
`SimpleGame.sln`을 열고 `Debug` 또는 `Release`, `x64` 또는 `x86`을 선택한다.

```powershell
msbuild SimpleGame.sln /m /p:Configuration=Release /p:Platform=x64
```

- `SimpleGame/Dependencies`의 GLFW 3.4 Win32 소스와 GLEW 2.2.0 소스를
  선택한 구성의 C 컴파일러로 빌드해 실행 파일에 정적으로 링크한다.
  소스 빌드에는 네트워크, CMake, vcpkg가 필요하지 않다.
- x64 Windows에서 x86을 빌드할 때도 x64 호스트 컴파일러를 기본으로 사용한다.
- 실행 파일을 옮길 때는 같은 빌드 출력의 `Shaders`, `resource`, `licenses` 폴더를
  함께 복사한다. OpenGL 3.3 호환 프로필을 지원하는 Windows 그래픽 드라이버가 필요하다.
- GLFW는 이식 가능한 창 라이브러리다. 현재 게임 빌드는 Win32 파일 API와
  GDI 글리프 래스터화를 사용하므로 지원 대상은 Windows다.

## 렌더링과 입력

- OpenGL 3.3 호환 프로필, 기존 셰이더, 모델 배치, 후처리, 한글 글리프 아틀라스를 사용한다.
- 시뮬레이션은 기존 16ms 타이머 간격과 50ms 델타 상한을 적용한다.
  프레임 사이에는 GLFW 이벤트를 대기하며 스왑 간격은 드라이버 기본값을 사용한다.
- WASD, 양쪽 Shift, Q, Space, 숫자키, 방향키, F1–F6을 확인한다.
  창을 최소화한 뒤 복귀했을 때 키가 눌린 상태로 남지 않는지 확인한다.
- 창 크기나 DPI가 달라진 뒤 게임 화면과 UI 비율을 확인한다.
  퀘스트 대사와 목표에서 한글, 숫자, `[E]`가 정상 표시되는지 확인한다.
  맑은 고딕으로 한글 글리프를 찾지 못하면 동봉한 나눔고딕을 게임 프로세스에서만 등록한다.

## 성능 비교

전환 전후 Release에서 같은 PC, 창 크기, 장면, 스왑 설정으로 비교한다.
F5로 프로파일링을 전환할 수 있다. 로그는 `%LOCALAPPDATA%\Emberwick` 아래의
`profile_*.jsonl`이다. 워밍업 뒤 동일 길이의 구간에서 드로 콜, 프레임 간격, GPU 시간,
`cpu.present.swap_wait_ms`를 함께 본다. `cpu.window.event_wait_ms`는 프레임 간 이벤트 대기를
포함하고, `cpu.window.event_poll_ms`는 대기 없이 이벤트를 처리한 구간이다.

## 소스와 라이선스

- [GLFW 3.4](https://github.com/glfw/glfw/tree/3.4)
- [GLEW 2.2.0](https://github.com/nigels-com/glew/releases/tag/glew-2.2.0)
- [나눔고딕](https://github.com/google/fonts/tree/main/ofl/nanumgothic)
