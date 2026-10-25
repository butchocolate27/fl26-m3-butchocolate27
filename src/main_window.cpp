#include "aiws/gui/main_window.hpp"
#include "aiws/gui/resettable.hpp"

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

#include <exception>

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

    connect(workspace_widget_, &WorkspaceWidget::build_corpus_requested,
            this, &MainWindow::build_corpus);

    connect(workspace_widget_, &WorkspaceWidget::document_selected,
            this, &MainWindow::document_selected);

    // Connect search events
    connect(search_widget_, &SearchWidget::search_requested,
            this, &MainWindow::search_requested);

    connect(search_widget_, &SearchWidget::query_changed,
            this, &MainWindow::query_changed);

    connect(search_widget_, &SearchWidget::source_document_requested,
            this, &MainWindow::source_document_requested);

    connect(search_widget_, &SearchWidget::selected_chunk_changed,
            term_inspector_widget_, &TermInspectorWidget::set_selected_chunk);

    // Connect inspector events
    connect(term_inspector_widget_, &TermInspectorWidget::term_inspection_requested,
            this, &MainWindow::term_inspection_requested);

    // Connect context events
    connect(context_widget_, &ContextWidget::build_context_requested,
            this, &MainWindow::build_context_requested);

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
    if (workspace_.documents().empty()) {
        return;
    }

    // Rebuild current corpus
    try {
        processing_core_.rebuild(workspace_);
    } catch (const std::exception& error) {
        invalidate_corpus();
        status_label_->setText("Build failed: " + QString::fromUtf8(error.what()));
        return;
    }

    corpus_current_ = true;
    clear_processing_outputs();

    // Show corpus status
    QString count = QString::number(processing_core_.chunk_count());
    workspace_widget_->set_corpus_status("Corpus: " + count + " chunks");
    status_label_->setText("Corpus built.");
    update_action_states();
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
    if (!corpus_current_ || query.trimmed().isEmpty()) {
        return;
    }

    // Run requested search
    try {
        std::vector<SearchResult> results = processing_core_.search(query.toStdString(), k);
        search_widget_->set_results(results);
        term_inspector_widget_->clear_statistics();
        context_widget_->clear_context();
        status_label_->setText("Search complete.");
    } catch (const std::exception& error) {
        clear_processing_outputs();
        status_label_->setText("Search failed: " + QString::fromUtf8(error.what()));
    }

    update_action_states();
}

void MainWindow::query_changed(const QString& query) {
    // Update action states
    (void)query;
    update_action_states();
}

void MainWindow::build_context_requested(int token_budget) {
    QString query = search_widget_->query();
    if (!corpus_current_ || query.trimmed().isEmpty()) {
        return;
    }

    // Build current context
    try {
        std::vector<ContextItem> items = processing_core_.build_context(
            query.toStdString(),
            search_widget_->result_count(),
            static_cast<std::size_t>(token_budget)
        );
        context_widget_->set_context(items);
        status_label_->setText("Context built.");
    } catch (const std::exception& error) {
        context_widget_->clear_context();
        status_label_->setText("Context failed: " + QString::fromUtf8(error.what()));
    }
}

void MainWindow::source_document_requested(const QString& document_id) {
    // Find source document
    const std::vector<Document>& documents = workspace_.documents();
    std::string id = document_id.toStdString();

    for (std::size_t row = 0; row < documents.size(); ++row) {
        if (documents[row].id() == id) {
            workspace_widget_->select_document(static_cast<int>(row));
            status_label_->setText("Source document shown.");
            return;
        }
    }

    status_label_->setText("Source document not found.");
}

void MainWindow::term_inspection_requested(const QString& term,
                                          const QString& chunk_id) {
    if (!corpus_current_ || term.trimmed().isEmpty()) {
        return;
    }

    // Read term statistics
    try {
        std::size_t df = processing_core_.document_frequency(term.toStdString());
        bool has_chunk = !chunk_id.isEmpty();
        std::size_t tf = 0;

        if (has_chunk) {
            tf = processing_core_.term_frequency(term.toStdString(), chunk_id.toStdString());
        }

        term_inspector_widget_->set_statistics(df, has_chunk, tf);
        status_label_->setText("Term inspected.");
    } catch (const std::exception& error) {
        term_inspector_widget_->clear_statistics();
        status_label_->setText("Inspection failed: " + QString::fromUtf8(error.what()));
    }
}

void MainWindow::reset_application() {
    // Clear workspace data
    workspace_.clear();
    corpus_current_ = false;

    // Reset all components
    Resettable* components[] = {
        workspace_widget_,
        search_widget_,
        term_inspector_widget_,
        context_widget_
    };

    for (Resettable* component : components) {
        component->reset();
    }

    update_action_states();
    status_label_->setText("Application reset.");
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
