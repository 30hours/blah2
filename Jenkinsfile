pipeline {
    agent any

    environment {
        GHCR_REGISTRY = "ghcr.io"
        GHCR_USERNAME = credentials('30hours')
        GHCR_TOKEN = credentials('ghcr-login')
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        stage('Build') {
            steps {
                script {
                    echo 'Building the project'
                    blah2 = docker.build("30hours/blah2", "--file ./Dockerfile .")
                    blah2_api = docker.build("30hours/blah2", "--file ./api/Dockerfile ./api")
                }
            }
        }
        stage('Test') {
            steps {
                echo 'Running tests'
            }
        }
        stage('Push') {
            steps {
                script {
                    echo 'Pushing the application'

                    docker.withRegistry("${GHCR_REGISTRY}", "${GHCR_TOKEN}") {
                        blah2.push()
                        blah2_api.push()
                    }
                }
            }
        }
    }
}
