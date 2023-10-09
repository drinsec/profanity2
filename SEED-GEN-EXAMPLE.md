# Use Argon2d with custom password

```
INITSEED=$(echo -n <PASSWORD> | ./argon2 profanity -d -m 16 -t 255 -p 6 -r)
./profanity2-x86_64 --gas -z "${INITSEED}"
```

# Use Argon2d with custom password and 32G RAM

```
INITSEED=$(echo -n <PASSWORD> | ./argon2 profanity -d -m 25 -t 255 -p 6 -r)
./profanity2-x86_64 --gas -z "${INITSEED}"
```