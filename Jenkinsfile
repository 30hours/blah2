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
                script {
                    echo 'Building the project'
                    blah2 = docker.build("30hours/blah2", "--file ./Dockerfile .")
                    blah2_api = docker.build("30hours/blah2", "--file ./api/Dockerfile .")
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
                echo 'Pushing the application'
            }
        }
    }
}
