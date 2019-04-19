Dockerfile.madoko is used to build a Docker image which we use to render the
P4Runtime specification (HTML & PDF) in Travis CI. The image can also be used
locally to build the specification without having to worry about installing all
the dependencies yourself.

Only maintainers of this repository need to build the Docker image when a new
image needs to be pushed to dockerhub. Contributors to the specification can
simply pull the image from dockerhub and don't have to worry about building the
image themselves. If you are a maintainer and you need to upload a new version
of the Docker image to to dockerhub, you will need the following commands:
```bash
docker build -t p4rt-madoko -f Dockerfile.madoko .
docker tag p4rt-madoko p4lang/p4rt-madoko:latest
docker push p4lang/p4rt-madoko:latest
```

Note that you need to have write permissions to the p4lang dockerhub repository
to push the image (and you need to be logged in).
