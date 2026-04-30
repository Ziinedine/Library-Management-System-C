# Library-Management-System-C
A library management system using AVL Trees and Heaps in C

# Library Management System (Advanced Data Structures)

A high-performance Library Management System implemented in **C**, focusing on efficient data organization and retrieval using advanced data structures. This project was developed as part of the **Data Structures** course.

## 🚀 Overview
The system manages books, members, loans, and reviews. It supports complex operations like genre-based categorization, member activity tracking, and a recommendation system based on book scores.

## 🛠️ Technical Specifications & Data Structures
The project implements several complex data structures to ensure optimal time complexity for various operations:

* **AVL Trees (Self-Balancing Binary Search Trees):** Used for indexing books by title within each genre, ensuring $O(\log n)$ search, insertion, and deletion.
* **Max Priority Queue (Binary Heap):** Implements a recommendation engine that suggests the top-rated books based on user reviews.
* **Singly & Doubly Linked Lists:** Used for managing library members and tracking active loans.
* **Dynamic Arrays (Look-up Tables):** Used for the "Display Slots" functionality to provide fast access to books per genre.
* **Custom Tokenizer:** A robust command-line parser for processing batch instructions from files or standard input.

## 📋 Features
* **Hierarchical Organization:** Books are grouped by genres, and each genre maintains its own balanced index.
* **Member Tracking:** Detailed logs of member activity, including loan counts and review history.
* **Recommendation System:** Dynamically calculates the best books to recommend using a Heap-based priority system.
* **Memory Management:** Full manual memory management with zero leaks, verified through extensive testing.

## 💻 How to Run
To compile the system using `gcc`:

```bash
gcc main.c library.c -o library_system
