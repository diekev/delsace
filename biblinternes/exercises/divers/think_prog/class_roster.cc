#include <iostream>

class StudentRecord {
	int m_grade;
	int m_student_ID;
	std::string m_name;

	bool isValidGrade(const int grade) const;

public:
	StudentRecord();
	StudentRecord(const int grade, const int ID, const std::string &name);
	~StudentRecord();

	int grade() const;
	void grade(const int grade);

	int studentID() const;
	void studentID(const int ID);

	std::string name() const;
	void name(const std::string &name);

	std::string letterGrade() const;
};

StudentRecord::StudentRecord()
	: StudentRecord(0, -1, "")
{}

StudentRecord::StudentRecord(const int grade, const int ID, const std::string &name)
	: m_grade(grade)
	, m_student_ID(ID)
	, m_name(name)
{}

StudentRecord::~StudentRecord()
{}

int StudentRecord::grade() const
{
	return m_grade;
}

void StudentRecord::grade(const int grade)
{
	if (isValidGrade(grade)) {
		m_grade = grade;
	}
}

int StudentRecord::studentID() const
{
	return m_student_ID;
}

void StudentRecord::studentID(const int ID)
{
	m_student_ID = ID;
}

std::string StudentRecord::name() const
{
	return m_name;
}

void StudentRecord::name(const std::string &name)
{
	m_name = name;
}

std::string StudentRecord::letterGrade() const
{
	if (!isValidGrade(m_grade)) {
		return "ERROR";
	}
	
	const int num_categories = 11;
	const std::string grade_letters[] = {"F", "D", "D+", "C-", "C", "C+", "B-", "B+", "A-", "A"};
	const int lowest_grade_score[] = {0, 60, 67, 70, 73, 77, 80, 83, 87, 90, 93};
	int category = 0;

	while (category < num_categories && lowest_grade_score[category] <= m_grade) {
		category++;
	}

	return grade_letters[category - 1];
}

bool StudentRecord::isValidGrade(const int grade) const
{
	return ((grade >= 0) && (grade <= 100));
}

class StudentCollection {
	struct StudentNode {
		StudentRecord m_student_data;
		StudentNode *next;
	};

	typedef StudentNode *StudentList;
	StudentList m_list_head;

	void deleteList(StudentList &list_ptr);
	StudentList copiedList(const StudentList original);

public:
	StudentCollection();
	StudentCollection(const StudentCollection &original);
	~StudentCollection();
	
	void addRecord(StudentRecord student);
	StudentRecord recordWithNumber(const int ID) const;
	void removeRecord(const int ID);

	StudentCollection &operator=(const StudentCollection &rhs);
};

StudentCollection::StudentCollection()
{
	m_list_head = nullptr;
}

StudentCollection::StudentCollection(const StudentCollection &original)
{
	m_list_head = copiedList(original.m_list_head);
}

StudentCollection::~StudentCollection()
{
	deleteList(m_list_head);
}

void StudentCollection::addRecord(const StudentRecord &student)
{
	StudentNode *node = new StudentNode;
	node->m_student_data = student;
	node->next = m_list_head;
	m_list_head = node;
}

StudentRecord StudentCollection::recordWithNumber(const int ID) const
{
	StudentNode *ptr = m_list_head;

	while (ptr != nullptr && ptr->m_student_data.studentID() != ID) {
		ptr = ptr->next;
	}

	if (ptr == nullptr) {
		return StudentRecord(-1, -1, "");
	}
	else {
		return ptr->m_student_data;
	}
}

void StudentCollection::removeRecord(const int ID)
{
	StudentNode *ptr = m_list_head;
	StudentNode *trailing = nullptr;

	while (ptr != nullptr && ptr->m_student_data.studentID() != ID) {
		trailing = ptr;
		ptr = ptr->next;
	}

	if (ptr == nullptr) {
		return;
	}

	if (trailing == nullptr) {
		m_list_head = m_list_head->next;
	}
	else {
		trailing->next = ptr->next;
	}

	delete ptr;
}

void StudentCollection::deleteList(StudentList &list_ptr)
{
	while (list_ptr != nullptr) {
		StudentNode *temp = list_ptr;
		list_ptr = list_ptr->next;
		delete temp;
	}
}

StudentCollection::StudentList StudentCollection::copiedList(const StudentList original)
{
	if (original == nullptr) {
		return nullptr;
	}

	StudentList new_list = new StudentNode;
	new_list->m_student_data = original->m_student_data;
	StudentNode *old_ptr = original->next;
	StudentNode *new_ptr = new_list;

	while (old_ptr != nullptr) {
		new_ptr->next = new StudentNode;
		new_ptr = new_ptr->next;
		new_ptr->m_student_data = old_ptr->m_student_data;
		old_ptr = old_ptr->next;
	}

	new_ptr->next = nullptr;

	return new_list;
}

StudentCollection &StudentCollection::operator=(const StudentCollection &rhs)
{
	if (this != &rhs) {
		deleteList(m_list_head);
		m_list_head = copiedList(rhs.m_list_head);
	}

	return *this;
}
