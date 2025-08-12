import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QProgressBar, QPushButton

class ProgressBarDemo(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PyQt5 Progress Bar Example")
        self.setGeometry(100, 100, 300, 100)
        layout = QVBoxLayout()

        self.progress = QProgressBar(self)
        self.progress.setMinimum(0)
        self.progress.setMaximum(100)
        layout.addWidget(self.progress)

        self.button = QPushButton("Increase", self)
        self.button.clicked.connect(self.increase_progress)
        layout.addWidget(self.button)

        self.setLayout(layout)
        self.value = 0

    def increase_progress(self):
        if self.value < 100:
            self.value += 10
            self.progress.setValue(self.value)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    demo = ProgressBarDemo()
    demo.show()
    sys.exit(app.exec_())
