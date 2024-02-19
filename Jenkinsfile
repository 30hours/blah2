pipeline {
    agent any

    environment {
        GHCR_REGISTRY = "ghcr.io"
        GHCR_TOKEN = credentials('ghcr-login')
        BLAH2_NAME = "30hours/blah2"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        stage('Build') {
            steps {
                echo 'Building the project'
                sh 'docker build -t $BLAH2_NAME .'
            }
        }
        stage('Test') {
            steps {
                echo 'Running tests'
            }
        }
        stage('Push') {
            steps {
                sh 'echo $GHCR_TOKEN_PSW | docker login ghcr.io -u $GHCR_TOKEN_USR --password-stdin'
                sh 'docker tag $BLAH2_NAME ghcr.io/$BLAH2_NAME'
                sh 'docker push ghcr.io/$BLAH2_NAME'
                sh 'docker logout'
            }
        }
    }
}
