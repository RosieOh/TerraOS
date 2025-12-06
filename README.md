# terraOS

32비트 하비 운영체제 프로젝트입니다. 교육 및 학습 목적으로 개발되었으며, Multiboot 호환 커널을 포함합니다.

## 📋 목차

- [주요 기능](#주요-기능)
- [요구사항](#요구사항)
- [빌드 방법](#빌드-방법)
- [실행 방법](#실행-방법)
- [프로젝트 구조](#프로젝트-구조)
- [개발 환경 설정](#개발-환경-설정)
- [사용 가능한 명령어](#사용-가능한-명령어)
- [기술 스택](#기술-스택)

## 🚀 주요 기능

### 핵심 기능
- **Multiboot 호환 부트로더**: GRUB을 통한 부팅 지원
- **VGA 텍스트 모드 콘솔**: 80x25 텍스트 출력
- **키보드 입력**: PS/2 키보드 인터럽트 처리
- **인터럽트 처리**: IDT, PIC, PIT 타이머 설정
- **메모리 관리**: 페이징, 힙 할당자, 프레임 할당자
- **멀티태스킹**: 협력형 및 선점형 멀티태스킹 지원
- **시스템 콜**: 유저 모드 프로그램을 위한 시스템 콜 인터페이스
- **ELF 로더**: ELF 형식 실행 파일 로드 및 실행

### 파일 시스템
- **RAMFS**: 메모리 기반 파일 시스템
- **VFS**: 가상 파일 시스템 레이어
- **DevFS**: 디바이스 파일 시스템

### 시스템 기능
- **쉘**: 기본 명령어 인터프리터
- **환경 변수**: 환경 변수 관리
- **시그널**: 프로세스 간 시그널 통신
- **파이프**: 프로세스 간 통신
- **경로 관리**: 현재 작업 디렉토리 관리
- **로깅**: 커널 로그 시스템
- **디버깅**: 스택 트레이스, 메모리 덤프, 레지스터 덤프

### 고급 기능
- **그래픽**: 그래픽 출력 지원
- **네트워크**: 네트워크 스택 (기본 구현)
- **SMP**: 대칭형 멀티프로세싱 지원
- **모니터링**: 시스템 모니터링 도구 (top, vmstat)

## 📦 요구사항

### 필수 도구
- **크로스 컴파일러**: `i686-elf-gcc` (GCC 크로스 컴파일러)
- **어셈블러**: `nasm`
- **에뮬레이터**: `qemu-system-i386`
- **Python 3**: 빌드 스크립트용

### 선택적 도구
- **GRUB**: ISO 이미지 생성용 (`grub-mkrescue`)

### macOS 설치 예시
```bash
# Homebrew를 사용한 설치
brew install i686-elf-gcc
brew install nasm
brew install qemu
brew install grub
```

### Linux 설치 예시
```bash
# Ubuntu/Debian
sudo apt-get install gcc-i686-elf nasm qemu-system-x86 grub-pc-bin

# Arch Linux
sudo pacman -S gcc-multilib nasm qemu-arch-extra grub
```

## 🔨 빌드 방법

### 전체 빌드
```bash
make all
```

이 명령은 다음을 수행합니다:
1. 유저 프로그램 빌드 (`user/` 디렉토리)
2. 커널 소스 컴파일
3. ELF 파일을 C 배열로 변환
4. 최종 커널 ELF 생성

### 개별 빌드 타겟
```bash
# 유저 프로그램만 빌드
make user-programs

# ISO 이미지 생성
make iso

# 빌드 정리
make clean          # 오브젝트 파일 삭제
make distclean      # 전체 빌드 디렉토리 삭제

# 툴체인 정보 확인
make toolchain-info
```

## ▶️ 실행 방법

### QEMU로 직접 실행 (권장)
```bash
make run
```

이 방법은 GRUB 없이 커널을 직접 로드합니다.

### ISO 이미지로 실행
```bash
make run-iso
```

이 방법은 GRUB 부트로더를 사용하여 ISO 이미지로 부팅합니다.

### 수동 실행
```bash
# 커널 ELF 직접 실행
qemu-system-i386 -display cocoa -kernel build/bin/kernel.elf

# ISO 이미지 실행
qemu-system-i386 -display cocoa -cdrom build/terraOS.iso
```

## 📁 프로젝트 구조

```
terraOS/
├── boot/              # 부트로더
│   └── boot.asm       # Multiboot 호환 부트로더
├── kernel/            # 커널 소스 코드
│   ├── kernel.c       # 메인 커널 코드
│   ├── interrupts.asm # 인터럽트 핸들러
│   ├── paging.c       # 페이징 관리
│   ├── tasks.c        # 태스크 스케줄링
│   ├── syscall.c      # 시스템 콜 처리
│   ├── gdt.c          # GDT 설정
│   ├── usermode.asm   # 유저 모드 전환
│   ├── elf.c          # ELF 로더
│   ├── ramfs.c        # RAM 파일 시스템
│   ├── memory.c       # 메모리 관리
│   ├── keyboard.c     # 키보드 드라이버
│   ├── log.c          # 로깅 시스템
│   ├── time.c         # 시간 관리
│   ├── env.c          # 환경 변수
│   ├── vfs.c          # 가상 파일 시스템
│   ├── signal.c       # 시그널 처리
│   ├── pipe.c         # 파이프
│   ├── network.c      # 네트워크 스택
│   ├── graphics.c     # 그래픽 출력
│   ├── smp.c          # SMP 지원
│   ├── shell.c        # 쉘 인터프리터
│   ├── path.c         # 경로 관리
│   ├── devfs.c        # 디바이스 파일 시스템
│   ├── heap.c         # 힙 할당자
│   ├── exec.c         # 실행 관리
│   ├── debug.c        # 디버깅 도구
│   ├── monitor.c      # 시스템 모니터링
│   ├── design.c       # UI 디자인 시스템
│   └── linker.ld      # 링커 스크립트
├── user/              # 유저 모드 프로그램
│   ├── hello.c        # 예제 프로그램
│   └── Makefile       # 유저 프로그램 빌드
├── tools/             # 빌드 도구
│   └── elf_to_c.py    # ELF를 C 배열로 변환
├── build/             # 빌드 출력 (자동 생성)
│   ├── bin/           # 오브젝트 파일 및 커널 ELF
│   └── iso/           # ISO 이미지 파일
├── Makefile           # 메인 빌드 파일
├── README.md          # 이 파일
└── .gitignore         # Git 무시 파일
```

## 🛠️ 개발 환경 설정

### 크로스 컴파일러 설정

크로스 컴파일러가 설치되어 있지 않은 경우, 다음 명령으로 확인:

```bash
i686-elf-gcc --version
```

설치되어 있지 않다면, 소스에서 빌드하거나 패키지 매니저를 사용하여 설치하세요.

### 디버깅

GDB를 사용한 디버깅:

```bash
# QEMU를 디버그 모드로 실행
qemu-system-i386 -display cocoa -kernel build/bin/kernel.elf -s -S

# 다른 터미널에서 GDB 연결
gdb build/bin/kernel.elf
(gdb) target remote localhost:1234
```

## 💻 사용 가능한 명령어

terraOS 쉘에서 사용할 수 있는 명령어:

### 기본 명령어
- `help` - 사용 가능한 명령어 목록 표시
- `clear` - 화면 지우기
- `echo <text>` - 텍스트 출력
- `ls` - RAMFS 파일 목록
- `cat <file>` - 파일 내용 출력
- `run <file>` - ELF 파일 실행

### 시스템 정보
- `meminfo` - 메모리 정보 표시
- `memmap` - 메모리 맵 정보
- `time` - 시스템 업타임
- `cpuinfo` - CPU 정보
- `ps` - 실행 중인 프로세스 목록
- `top` - 프로세스 모니터
- `vmstat` - 시스템 통계

### 파일 시스템
- `mkdir <dir>` - 디렉토리 생성
- `rm <file>` - 파일 삭제
- `cd <dir>` - 디렉토리 변경
- `pwd` - 현재 작업 디렉토리 출력
- `dev` - 디바이스 목록

### 환경 변수
- `export VAR=value` - 환경 변수 설정
- `env` - 모든 환경 변수 표시

### 멀티태스킹 데모
- `demo-coop` - 협력형 멀티태스킹 데모
- `demo-preempt` - 선점형 멀티태스킹 데모
- `demo-usermode` - 유저 모드 (Ring 3) 데모

### 시스템 관리
- `nice <pid> <value>` - 프로세스 우선순위 설정
- `kill <pid>` - 프로세스에 시그널 전송
- `jobs` - 백그라운드 작업 목록
- `fg <job>` - 작업을 포그라운드로
- `bg <job>` - 작업을 백그라운드로

### 디버깅
- `heapinfo` - 힙 할당 정보
- `stacktrace` - 스택 트레이스
- `memdump <addr> <size>` - 메모리 덤프
- `regs` - 레지스터 덤프
- `log` - 커널 로그 표시
- `syscall-test` - 시스템 콜 테스트

## 🔧 기술 스택

- **언어**: C (GNU99), x86 Assembly (NASM)
- **아키텍처**: x86 (32비트)
- **부트로더**: Multiboot 1
- **빌드 시스템**: Make
- **에뮬레이터**: QEMU
- **크로스 컴파일러**: i686-elf-gcc

## 📝 참고사항

- 이 프로젝트는 교육 목적으로 개발되었습니다.
- 실제 운영 환경에서 사용하기에는 부족한 부분이 많습니다.
- 버그 리포트와 기여를 환영합니다.

## 📄 라이선스

이 프로젝트의 라이선스는 명시되지 않았습니다. 사용 시 주의하세요.

## 🤝 기여

이슈 리포트와 풀 리퀘스트를 환영합니다. 프로젝트를 포크하여 개선사항을 제안해주세요.

---

**terraOS** - 32비트 하비 운영체제 프로젝트

