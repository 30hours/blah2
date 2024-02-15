pipeline {
    agent any

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        stage('Build') {
            steps {
                echo 'Building the project'
                sh "docker build -t 30hours/blah2 -f Dockerfile ."
                sh "docker build -t 30hours/blah2_api -f ./api/Dockerfile ."
            }
        }
        stage('Test') {
            steps {
                echo 'Running tests'
            }
        }
        stage('Push') {
            steps {
                echo 'Pushing the application'
            }
        }
    }
}
