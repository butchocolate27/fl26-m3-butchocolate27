#include "aiws/gui/main_window.hpp"

#include "aiws/gui/workspace_widget.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/context_widget.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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

    // Connect workspace events
    connect(workspace_widget_, &WorkspaceWidget::add_document_requested,
            this, &MainWindow::add_document);

    connect(workspace_widget_, &WorkspaceWidget::document_selected,
            this, &MainWindow::document_selected);

    // Connect footer buttons
    connect(reset_button_, &QPushButton::clicked,
            this, &MainWindow::reset_application);

    connect(exit_button_, &QPushButton::clicked,
            this, &MainWindow::exit_application);

    update_action_states();
}

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_ = callback;
}

void MainWindow::add_document() {
    // Choose document file
    QString path = QFileDialog::getOpenFileName(
        this, "Add Document", QString(),
        "Text Files (*.txt);;All Files (*)"
    );

    if (path.isEmpty()) {
        return;
    }

    QFileInfo info(path);
    QString id = info.absoluteFilePath();
    QString title = info.fileName();

    // Check existing documents
    for (const Document& document : workspace_.documents()) {
        if (document.id() == id.toStdString()) {
            status_label_->setText("Document already added.");
            return;
        }
    }

    // Read document text
    QFile file(id);
    if (!file.open(QIODevice::ReadOnly)) {
        status_label_->setText("Could not open file.");
        return;
    }

    QString text = QString::fromUtf8(file.readAll());
    if (file.error() != QFileDevice::NoError) {
        status_label_->setText("Could not read file.");
        return;
    }

    // Update backend first
    Document document(id.toStdString(), title.toStdString(), text.toStdString());
    workspace_.add_document(document);
    invalidate_corpus();

    workspace_widget_->add_document(title);
    status_label_->setText("Document added: " + title);
}

void MainWindow::build_corpus() {
    // TODO
}

void MainWindow::document_selected(int row) {
    const std::vector<Document>& documents = workspace_.documents();

    if (row < 0 || static_cast<std::size_t>(row) >= documents.size()) {
        workspace_widget_->clear_selected_document();
        return;
    }

    // Show selected document
    const Document& document = documents[row];
    workspace_widget_->show_selected_document(
        QString::fromStdString(document.title()),
        QString::fromStdString(document.text())
    );
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
    workspace_widget_->set_build_corpus_enabled(!workspace_.documents().empty());

    bool can_search = corpus_current_ && search_widget_->has_query();
    search_widget_->set_search_enabled(can_search);
    context_widget_->set_build_context_enabled(can_search);
    term_inspector_widget_->set_inspection_enabled(corpus_current_);
}

void MainWindow::invalidate_corpus() {
    // Invalidate old corpus
    corpus_current_ = false;
    workspace_widget_->set_corpus_status("Corpus: Not built");
    clear_processing_outputs();
    update_action_states();
}

void MainWindow::clear_processing_outputs() {
    // Clear old output
    search_widget_->clear_results();
    term_inspector_widget_->set_selected_chunk(QString());
    term_inspector_widget_->clear_statistics();
    context_widget_->clear_context();
}

}  // namespace aiws::gui
