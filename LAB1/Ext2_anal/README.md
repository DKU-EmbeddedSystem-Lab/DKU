LAB1 실습 중 ramdisk.ko 모듈이 잘 동작하지 않는 분들께서는

insbrd.sh를 실행하여 후 생기는 /dev/ramdisk를 사용하여 실습 진행하실 수 있습니다.

사용이 끝나면 rmbrd.sh를 실행하여 모듈을 제거할 수 있습니다.
```
./insbrd.sh //램디스크 모듈 생성
fdisk -l /dev/ramdisk //모듈이 잘 생성되었는지 확인 가능
//실습 이어서 진행
./rmbrd.sh //램디스크 모듈 제거
```
