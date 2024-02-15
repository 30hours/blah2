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
                docker.build("30hours/blah2", "--file ./Dockerfile .")
                docker.build("30hours/blah2", "--file ./api/Dockerfile .")
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
