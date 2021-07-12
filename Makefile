
.PHONY: build
build:
	CC=arm-linux-gnueabihf-gcc GOOS=linux GOARCH=arm GOARM=6 CGO_ENABLED=1 go build ./cmd/hmat/...

.PHONY: send
send:
	rsync -a ./ pi@192.168.0.38:/home/pi/hmac




