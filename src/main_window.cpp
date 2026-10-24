#include "aiws/gui/main_window.hpp"

#include "aiws/gui/workspace_widget.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/context_widget.hpp"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace aiws::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("AI Workspace");
    resize(900, 900);

    // Create main layout
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setSpacing(10);

    // Add window header
    QLabel* header = new QLabel("AI Workspace", central);
    header->setMargin(12);
    header->setStyleSheet(
        "background-color: #6b2032; color: white; font-size: 24px;"
    );
    layout->addWidget(header);

    // Add workspace area
    workspace_widget_ = new WorkspaceWidget(central);
    layout->addWidget(workspace_widget_, 3);

    // Add search area
    search_widget_ = new SearchWidget(central);
    layout->addWidget(search_widget_, 4);

    // Add bottom areas
    term_inspector_widget_ = new TermInspectorWidget(central);
    context_widget_ = new ContextWidget(central);

    QHBoxLayout* bottom = new QHBoxLayout;
    bottom->addWidget(term_inspector_widget_, 1);
    bottom->addWidget(context_widget_, 1);
    layout->addLayout(bottom, 2);

    // Add status row
    QHBoxLayout* footer = new QHBoxLayout;
    footer->addWidget(new QLabel("Status:", central));

    status_label_ = new QLabel("Ready", central);
    footer->addWidget(status_label_, 1);

    reset_button_ = new QPushButton("Reset", central);
    reset_button_->setStyleSheet(
        "background-color: #d6a43c; color: black; padding: 6px 20px;"
    );
    footer->addWidget(reset_button_);

    exit_button_ = new QPushButton("Exit", central);
    exit_button_->setStyleSheet(
        "background-color: #3566a2; color: white; padding: 6px 20px;"
    );
    footer->addWidget(exit_button_);

    layout->addLayout(footer);

    // Connect footer buttons
    connect(reset_button_, &QPushButton::clicked,
            this, &MainWindow::reset_application);

    connect(exit_button_, &QPushButton::clicked,
            this, &MainWindow::exit_application);
}

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_ = callback;
}

void MainWindow::add_document() {
    // TODO
}

void MainWindow::build_corpus() {
    // TODO
}

void MainWindow::document_selected(int row) {
    // TODO
    (void)row;
}

void MainWindow::search_requested(const QString& query, int k) {
    // TODO
    (void)query;
    (void)k;
}

void MainWindow::query_changed(const QString& query) {
    // TODO
    (void)query;
}

void MainWindow::build_context_requested(int token_budget) {
    // TODO
    (void)token_budget;
}

void MainWindow::source_document_requested(const QString& document_id) {
    // TODO
    (void)document_id;
}

void MainWindow::term_inspection_requested(const QString& term,
                                          const QString& chunk_id) {
    // TODO
    (void)term;
    (void)chunk_id;
}

void MainWindow::reset_application() {
    // TODO
}

void MainWindow::exit_application() {
    if (exit_callback_ != nullptr) {
        exit_callback_(this);
    }
}

void MainWindow::update_action_states() {
    // TODO
}

void MainWindow::invalidate_corpus() {
    // TODO
}

void MainWindow::clear_processing_outputs() {
    // TODO
}

}  // namespace aiws::gui
