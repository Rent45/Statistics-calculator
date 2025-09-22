#include <iostream>
#include <utility>
#include <conio.h>  // For _kbhit() and _getch()
#include <windows.h>
#include <ctime>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <specialfunctions.h>

#define KEY_ARROW_UP 72
#define KEY_ARROW_DOWN 80
#define KEY_ENTER 13
#define KEY_ESC 27
#define Normal 0b00000111
#define Selected 0b00001111
#define NonSelect 0b00001000
#define MAX 30
#define NO_LIMIT (-1)
#define DECIMAL_PLACES 2

using namespace std;
using alglib::invincompletegammac;    // Inverse incomplete gamma (chi-square)
using alglib::inverf;                // Inverse error function (for z-scores)
using alglib::invstudenttdistribution; // Inverse t-distribution CDF

/*
// Helper function to determine the sign of a number.
double sign(const double x) {
    return (x > 0) - (x < 0);
}

// Newton refinement approximation of the inverse error function.
double NewtonRefinementInverseError(const double x) {
    double r;
    double a[] = {0.886226899, -1.645349621, 0.914624893, -0.140543331};
    double b[] = {1, -2.118377725, 1.442710462, -0.329097515, 0.012229801};
    double c[] = {-1.970840454, -1.62490649, 3.429567803, 1.641345311};
    double d[] = {1, 3.543889200, 1.637067800};

    double z = sign(x) * x;

    if (z <= 0.7) {
        double x2 = z * z;
        r = z * (((a[3] * x2 + a[2]) * x2 + a[1]) * x2 + a[0]);
        r /= ((((b[4] * x2 + b[3]) * x2 + b[2]) * x2 + b[1]) * x2 + b[0]);
    } else {
        double y = sqrt(-log((1 - z) / 2));
        r = (((c[3] * y + c[2]) * y + c[1]) * y + c[0]);
        r /= ((d[2] * y + d[1]) * y + d[0]);
    }

    r = r * sign(x);
    z = z * sign(x);

    // One Newton refinement iteration:
    r -= (erf(r) - z) / (2 / sqrt(M_PI) * exp(-r * r));

    return r;
}*/

// Validate that a string represents a valid double
bool isValidDouble(const string& str) {
    istringstream iss(str);
    double d;
    char c;
    return iss >> d && true;  // success if: double parsed, but no leftover junk
}

// Limit console buffer height to current window height
void setConsoleHeightLimit(HANDLE hConsole) {
    if (hConsole == INVALID_HANDLE_VALUE) {
        cerr << "Error: Unable to obtain console handle." << endl;
    }

    // Retrieve current console screen buffer info
    CONSOLE_SCREEN_BUFFER_INFO csbi = {};
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        cerr << "Error: Unable to retrieve console screen buffer info." << endl;
    }
    // Calculate the visible window height in rows
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Set the new buffer height to screen height with explicit casting to SHORT
    COORD newSize;
    newSize.X = csbi.dwSize.X;  // keep the current width
    newSize.Y = static_cast<SHORT>(windowHeight); // explicit cast to avoid narrowing conversion

    if (!SetConsoleScreenBufferSize(hConsole, newSize)) {
        cerr << "Error: Unable to set console screen buffer size." << endl;
    }
}

void printMatrix(const string data[][MAX], const int rows, const int cols, const int cellWidth, const bool showRowLabels = false)
{
    for (int i = 0; i < rows; ++i) {
        if (showRowLabels) {
            // cout << "Variable " << (i+1) << ": ";
            cout << ((i==0)?"Actual":"Expected") << ": ";
        }
        for (int j = 0; j < cols; ++j) {
            cout << setw(cellWidth) << data[i][j] << ' ';
        }
        cout << endl;
    }
    cout << endl;
}

class Option {
    string name;
    int offset;
    bool visible, selectable;
    public:
    Option() : name(""), offset(0), visible(false), selectable(true) {}
    Option(const Option& other):name(other.name),offset(other.offset),visible(other.visible),selectable(other.selectable){}
    // Main parameterized ctor: moves in name for performance, sets indent, visibility & selectability
    Option(string  Name, int Off, bool Visibility, bool Selectability):name(move(Name)),offset(Off), visible(Visibility), selectable(Selectability){};
    Option(string  Name, int Off, bool Visibility):name(move(Name)),offset(Off), visible(Visibility), selectable(true){};
    Option(string  Name, int Off):name(move(Name)),offset(Off), visible(true), selectable(true){};
    explicit Option(string Name):name(move(Name)),offset(0), visible(true), selectable(true){};
    const string& getName(){return name;}     // Exposes pointer to the internal name – used by searchParameter()
    int getOffset() const{return offset;}
    bool isVisible() const{return visible;}
    bool isSelectable() const{return selectable;}
    void toggleVisibility(){visible = !visible;}
};
Option answerMenu[] = {
    Option("What do you want?", 0, true, false),
    Option("To check my answer", 1),
    Option("To view the answer", 1)
};
Option expandables[3] = {Option("How do you want to select parameters?", 2, true, false),
                        Option("Manually", 2, true),
                        Option("Randomly", 2, true)};

class Parameter {
    string name;
    double value;
    bool accounted[7];
    int max, min;
    double actmax, actmin;
    public:
    explicit Parameter(string Name, const bool first, const bool second, const bool third, const bool fourth, const bool first1, const bool second1, const bool third1, const int Min, const int Max, const double MIN, const double MAAX):name(move(Name)), value(0),accounted{first, second, third, fourth, first1, second1, third1},max(Max),min(Min),actmax(MAAX),actmin(MIN){}
    const string *getName() const{return &name;}
    double getValue() const{return value;}
    bool isAccounted(int i) const{return accounted[i];}
    int getUpper() const{return max;}
    int getLower() const{return min;}
    string getLimits() const;
    void setValue(const double i){value=i;}
    static void generateVaue(Parameter *array, int size);
};
Parameter parameters[8] = {Parameter("sample size", true, true, false, true, true, true, false,1,NO_LIMIT,10, 300),
                        Parameter("population proportion", false, false, false, true, true, false, false,0,1, 0.25, 0.75),
                        Parameter("sample proportion", false, false, false, true, false, true, true,0,1, 0.25, 0.75),
                        Parameter("sample mean value", false, true, false, false, false, false, false,NO_LIMIT,NO_LIMIT, -100, 100),
                        Parameter("population standard deviation", true, false, true, false, false, false, false,0,NO_LIMIT, 0.5, 10),
                        Parameter("sample standard deviation", false, true, false, false, false, false, false,0,NO_LIMIT, 0, 1),
                        Parameter("margin of error", true, false, true, false, true, false, true,0,NO_LIMIT, 0, .75),
                        Parameter("confidence level", false, true, true, true, false, true, true,0,1,0.9,0.999)};
Parameter& searchParameter(const string& str) {
    for (int i = 0; i < 8; i++) if(*parameters[i].getName()==str) return parameters[i];
    return parameters[0];
}
string Parameter::getLimits() const {
    ostringstream oss;
    if (min != NO_LIMIT) oss << ", the lowest value should be " << min;
    if (max != NO_LIMIT) oss << ", the highest value should be " << max;
    return oss.str();
}
void Parameter::generateVaue(Parameter* array, const int size) {
    // First element: uniform rand in [actmin,actmax), floored to integer part
    array->setValue(floor(static_cast<double>(rand())/RAND_MAX * ((array->actmax)-(array->actmin)))+(array->actmin));
    // Subsequent elements: uniform rand in [actmin,actmax], rounded to 2 decimal places
    for (int i = 1; i < size; i++) {
        (array+i)->setValue(round((static_cast<double>(rand())/RAND_MAX * (((array+i)->actmax)-((array+i)->actmin))+((array+i)->actmin))*pow(10,DECIMAL_PLACES))/pow(10,DECIMAL_PLACES));
    }
    // Ensure last parameter (confidence level) never reaches 1.0 exactly
    if ((array+size-1)->value>=1) (array+size-1)->value=0.999;
}



double answer[2] = {0,0};
// --- Shared input data (visible to Solve) ---
static string inputData[MAX][MAX];  // external storage for Solve() for last type of problem
static int inputLines = 0;                       // number of rows
static int inputWords = 0;                       // number of cols

void displayMenu(HANDLE h, const int selectedIndex, Option options[], const int size) {
    setConsoleHeightLimit(h);
    system("cls");  // Clear the console
    for (int i = 0; i < size; i++) {
        if (i == selectedIndex){
            SetConsoleTextAttribute(h, Selected);
            for (int j = 1; j<=options[i].getOffset(); j++){cout<<"   ";};
            cout << " < " << options[i].getName() << " > " << endl;  // Highlight selected option
        }
        else if (options[i].isVisible()&&options[i].isSelectable()){
            for (int j = 1; j<=options[i].getOffset(); j++){cout<<"   ";};
            cout << "   " << options[i].getName() << "   " << endl;  // Regular option
        }
        else if (options[i].isVisible()&&!options[i].isSelectable()) {
            SetConsoleTextAttribute(h, NonSelect);
            for (int j = 1; j<=options[i].getOffset(); j++){cout<<"   ";};
            cout << "   " << options[i].getName() << "   " << endl;  // Non-selectable option
        }
        SetConsoleTextAttribute(h, Normal);
    }
    cout << endl;
}

// Shows options[0..size) on the console, lets the user move a highlight up/down and confirm with Enter.
// Returns the final selected index (or –1 if they hit Esc).
int navigateMenu(HANDLE hConsole, Option options[], const int size, int I = 1){
    const bool keepGoing = true;
    while (keepGoing) {
        displayMenu(hConsole, I, options, size);
        int key = _getch();
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) {  // Arrow keys return two values
            do{
                I=(I+((key == KEY_ARROW_DOWN)?(1):(size-1)))%(size); // Move
            }while (!((options[I].isVisible())&&(options[I].isSelectable())));
        }
        else if (key == KEY_ENTER)      return I;
        else if (key == KEY_ESC)        return NO_LIMIT;
        // otherwise ignore and re-draw
    }
    return I;
}

void showHelp() {
    system("cls");
    ifstream my1("../Types of problems.txt");
    if (!my1.is_open()) cout<<"File could not be opened."<<endl;
    else {
        string buff;
        while (getline (my1, buff)) {
            // Output the text from the file
            cout << buff << endl;
        }
    };
    my1.close();
    getch();
}

void showAuthor() {
    system("cls");
    ifstream my1("../Author.txt");
    if (!my1.is_open()) cout<<"File could not be opened."<<endl;
    else {
        string buff;
        while (getline (my1, buff)) {
            // Output the text from the file
            cout << buff;
        }
    };
    my1.close();
    getch();
}

void toggleManual(int selectedIndex, int maxmenuSize, Option menu[]) {
    for (int i = 1; i <= 2; i++, selectedIndex=(selectedIndex+1)%(maxmenuSize), menu[selectedIndex].toggleVisibility()){};
    do {
        selectedIndex=(selectedIndex+1)%(maxmenuSize);
        if (menu[selectedIndex].getName().compare(0, 4, "Type") == 0) menu[selectedIndex].toggleVisibility();
    } while(menu[selectedIndex].getName()!="Select the problem manually");
}

void Expand(int selectedIndex, int maxmenuSize, Option menu[]) {
    for(int i = maxmenuSize-1; i>=selectedIndex+3+1; i--) {
        menu[i]=menu[i-3];
    }
    for (int i = 1; i <= 3; i++) {
        menu[selectedIndex+i]=expandables[i-1];
    }
}

void outputProblem (int index, const string& sign) {
    cout << endl << "The problem:"<<endl;
    ifstream my1("../Types inside code.txt");
    if (!my1.is_open()) cout << "Could not open the file!" << endl;
    string line;
    for (int i = 0;  i < index; i++) getline(my1, line);
    my1.close();
    for (int i = 0; i < 8; i++) {
        if (parameters[i].isAccounted(index-1)) {
            size_t pos = line.find("TYPE");
            ostringstream oss;
            oss << fixed << setprecision(2) << parameters[i].getValue();
            line.replace(pos, 4, oss.str());
        };
    };
    if (index==4) {
        line.replace(line.find("SIGN"), 4, sign);
    }
    cout << line << endl;
}

int NavigateMenuAndOutputProblem (HANDLE Console, const int index, const string& choice) {
    bool flag = true;
    int selectedChoice = 1;
    while (flag) {
        displayMenu(Console, selectedChoice, answerMenu, 3);
        outputProblem(index, choice);
        int key = _getch();  // Wait for key press
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) {  // Arrow keys return two values
            selectedChoice=selectedChoice+((selectedChoice==1)?1:-1);
        } else if (key == KEY_ENTER) {
            flag=false;
        } else if (key == KEY_ESC) {
            break;  // Exit program
        } else {
            // selectedIndex=selectedIndex+((selectedIndex==1)?1:-1);
        }
    }
    return selectedChoice;
}

// return true if the user’s response matches the internally computed answer
bool checkUserAnswer(const int problemType, const double answer[]) {
    bool Result = false;
    switch (problemType) {
        case 1: case 3: case 5: case 7:
        {
            // single‐value problems
            cout << "Enter your answer: ";
            double user;
            cin >> user;
            Result = lround(user * pow(10,DECIMAL_PLACES)) == lround(answer[0] * pow(10,DECIMAL_PLACES));
            if (Result) cout<<endl<<"You are correct! Congratulations!";
            else cout<<endl<<"Unfortunately, you are wrong. The answer is: "<<*answer<<endl;
            break;
        }
        case 2: case 6: {
            // two‐value interval problems
            cout << "Enter lower and upper bounds: ";
            double lo, hi;
            cin >> lo >> hi;
            Result = lround(lo*pow(10,DECIMAL_PLACES)) == lround(answer[0]*pow(10,DECIMAL_PLACES))
                 && lround(hi*pow(10,DECIMAL_PLACES)) == lround(answer[1]*pow(10,DECIMAL_PLACES));
            if (Result) cout<<endl<<"You are correct! Congratulations!";
            else cout<<endl<<"Unfortunately, you are wrong. The answer is: "<<setprecision(2)<<*answer << ", " << answer[1];
            break;
        }
        case 4: {
            // hypothesis‐test yes/no
            cout << "Reject null hypothesis? (yes/no): ";
            string resp;
            cin >> resp;
            for (int i = 0; resp[i]; i++) {resp[i]=static_cast<char>(tolower(resp[i]));};
            bool userSaysReject = (resp == "yes");
            Result = ((userSaysReject)&&(*answer==1))||((!userSaysReject)&&(*answer==0));
            if(Result) cout<<endl<<"You are correct! Congratulations!";
            else cout<<endl<<"Unfortunately, you are wrong. We actually "<<((*answer==1)?"can.":"can not.");
            break;
        }
        case 8: case 9: {
            string presume;
            if(*answer>1) {
                cout << "Is the second variable a good model for the first? Enter yes or no: ";
                cin >> presume;
                for (int i = 0; presume[i]; i++) {presume[i]=static_cast<char>(tolower(presume[i]));};
                Result = ((presume=="yes")&&(*answer==3))||((presume=="no")&&(*answer==2));
                if (Result) cout<<endl<<"You are correct! Congratulations!";
                else cout<<endl<<"Unfortunately, you are wrong. They actually "<<((*answer==3)?"are.":"are not.");
            }
            else {
                cout << "Are the variables correlated? Enter yes or no: ";
                cin >> presume;
                for (int i = 0; presume[i]; i++) {presume[i]=static_cast<char>(tolower(presume[i]));};
                Result = ((presume=="yes")&&(*answer==1))||((presume=="no")&&(*answer==0));
                if(Result) cout<<endl<<"You are correct! Congratulations!";
                else cout<<endl<<"Unfortunately, you are wrong. They actually "<<((*answer==1)?"are.":"are not.");
            }
            break;
        }
        default: break;
    }
    return Result;
}


void Solve (const int ProblemNumber, const string& choice) {
    double z = sqrt(2)*inverf(searchParameter("confidence level").getValue(),alglib::xparams{});
    switch (ProblemNumber) {
        case 1: z = sqrt(searchParameter("sample size").getValue())*(searchParameter("margin of error").getValue())/(searchParameter("population standard deviation").getValue()); *answer = erf(z/sqrt(2)); break;
        case 5: z = (searchParameter("margin of error").getValue())*sqrt((searchParameter("sample size").getValue())/((searchParameter("population proportion").getValue())*(1-(searchParameter("population proportion").getValue())))); *answer = erf(z/sqrt(2)); break;
        case 2: { const double t = invstudenttdistribution(static_cast<int>(searchParameter("sample size").getValue()-1),(searchParameter("confidence level").getValue()+1)/2,alglib::xparams{}); *answer = searchParameter("sample mean value").getValue()-t*searchParameter("sample standard deviation").getValue()/sqrt(searchParameter("sample size").getValue()); *(answer+1) = searchParameter("sample mean value").getValue()+t*searchParameter("sample standard deviation").getValue()/sqrt(searchParameter("sample size").getValue()); break;}
        case 6: *answer = searchParameter("sample proportion").getValue()-z*sqrt(searchParameter("sample proportion").getValue()*(1-searchParameter("sample proportion").getValue())/searchParameter("sample size").getValue()); *(answer+1) = searchParameter("sample proportion").getValue()+z*sqrt(searchParameter("sample proportion").getValue()*(1-searchParameter("sample proportion").getValue())/searchParameter("sample size").getValue()); break;
        case 3: *answer = ceil(pow(z*searchParameter("population standard deviation").getValue()/searchParameter("margin of error").getValue(),2)); break;
        case 7: *answer = ceil(pow(z/searchParameter("margin of error").getValue(),2)*searchParameter("sample proportion").getValue()*(1-searchParameter("sample proportion").getValue())); break;
        case 4: {
            const double SE = sqrt((searchParameter("population proportion").getValue())*(1-(searchParameter("population proportion").getValue()))/searchParameter("sample size").getValue());
            const double zAct = (searchParameter("sample proportion").getValue()-searchParameter("population proportion").getValue())/SE;
            if (choice=="=") *answer = (abs(zAct)>z)?1:0;
            else z = sqrt(2)*inverf(2*searchParameter("confidence level").getValue()-1,alglib::xparams{});
            if (choice=="<=") *answer = (zAct>z)?1:0;
            if (choice==">=") *answer = (zAct<-z)?1:0;
            break;
        }
        case 8: {
            size_t pos;
            double chi_square = 0, chi_square_Act = 0, sum = 0;
            double expected[inputLines][inputWords] = {0}, marginal_bottom[inputWords] = {0}, marginal_left[inputLines] = {0};
            double degrees_of_freedom = (inputWords-1)*(inputLines-1);
            chi_square = 2*invincompletegammac(degrees_of_freedom/2,1-searchParameter("confidence level").getValue(),alglib::xparams{});
            for (int i=0; i<inputLines; i++) for (int j=0; j<inputWords; j++) {
                sum+=stod(inputData[i][j], &pos);
                marginal_bottom[j]+=stod(inputData[i][j], &pos);
                marginal_left[i]+=stod(inputData[i][j], &pos);
            }
            for (int i=0; i<inputLines; i++) for (int j=0; j<inputWords; j++) {
                expected[i][j] = marginal_bottom[j]*marginal_left[i]/sum;
                chi_square_Act+=pow(expected[i][j]-stod(inputData[i][j], &pos),2)/expected[i][j];
            }
            if(chi_square_Act>chi_square) (*answer)=1;
            else (*answer)=0;
            break;
        }
        case 9: {
            bool flag=true;
            size_t pos;
            for (int i = 0; i < inputWords; i++) {
                if (!isValidDouble(inputData[0][i])) {
                    flag = false;
                    break;
                }
            }
            double chi_square = 0, chi_square_Act = 0;
            double degrees_of_freedom = inputWords-1;
            chi_square = 2*invincompletegammac(degrees_of_freedom/2,1-searchParameter("confidence level").getValue(),alglib::xparams{});
            if (flag) {
                for (int i=0; i<inputWords; i++) chi_square_Act+=pow((stod(inputData[0][i], &pos)-stod(inputData[1][i], &pos)),2)/stod(inputData[0][i], &pos);
            } else {
                double expected_value = 0;
                for (int i=0; i<inputWords; i++) expected_value+=stod(inputData[1][i], &pos);
                expected_value /= inputWords;
                for (int i=0; i<inputWords; i++) chi_square_Act+=pow(expected_value-stod(inputData[1][i], &pos),2)/expected_value;
            }
            if(chi_square_Act>chi_square&&flag) {(*answer)=2; break;}
            if(chi_square_Act>chi_square&&!flag) {(*answer)=1; break;}
            if(flag) {(*answer)=3; break;}
            (*answer)=0;
            break;
        }
        default: break;
    };
};

string expectedOutput(const double *expected, const double* marginal_bottom, const double* marginal_left, const double sum) {
    ostringstream result;
    //Expected Appetizer/Dessert: (112 ⋅ 120)/200 = 67.2
    for (int i=0; i<inputLines; i++) {
        for (int j=0; j<inputWords; j++) {
            result << "Expected (" << i << ", " << j << "): ((";
            for (int k=0; k<inputWords; k++) {
                result << ((k==0)?"":"+") << inputData[i][k];
            }
            result << ")*(";
            for (int k=0; k<inputLines; k++) {
                result << ((k==0)?"":"+") << inputData[k][j];
            }
            result << "))/" << sum << " = " << marginal_left[i] << "*" << marginal_bottom[j] << "/" << sum << " = " << expected[i*inputWords+j] << endl;
        }
    }
    return result.str();
}

string newTable(const double *expected, const int width) {
    ostringstream result;
    for(int i = 0; i < inputLines; i++){
        for(int j = 0; j < inputWords; j++)
            result << setw(2*width) << inputData[i][j] << "(" << expected[i*inputWords+j] << ") ";
        result << endl;
    };result<<endl;
    return result.str();
}

string chiSum (const double *expected, const int flag) {
    ostringstream result;
    switch (flag) {
        case 0: {
            double sum_arr[inputLines*inputWords];
            for(int i=0; i<inputLines; i++) {
                for(int j=0; j<inputWords; j++) {
                    sum_arr[i*inputWords+j] = pow(stod(inputData[i][j])-expected[i*inputWords+j],2)/expected[i*inputWords+j];
                }
            }
            result << "𝜒² = ";
            for(int i = 0; i < inputLines; i++){
                for(int j = 0; j < inputWords; j++)
                    result << setprecision(2) << ((i+j==0)?"":"+ ") << "(" << inputData[i][j] << "-" << expected[i*inputWords+j] << ")²/" << expected[i*inputWords+j] << " ";
            };result<<endl;
            result << "𝜒² ≈ ";
            for(int i = 0; i < inputLines*inputWords; i++){
                result << ((i==0)?"":"+ ") << sum_arr[i] << " ";
            };
            break;
        }
        case 1: {
            double sum_arr[inputWords];
            for(int j=0; j<inputWords; j++) {
                sum_arr[j] = pow(stod(inputData[1][j])-(*expected),2)/(*expected);
            }
            result << "𝜒² = ";
            for(int j = 0; j < inputWords; j++) result << setprecision(2) << ((j==0)?"":"+ ") << "(" << inputData[1][j] << "-" << *expected << ")²/" << *expected << " ";
            result<<endl;
            result << "𝜒² ≈ ";
            for(int i = 0; i < inputWords; i++){
                result << ((i==0)?"":"+ ") << sum_arr[i] << " ";
            };
            break;
        }
        case 2: {
            double sum_arr[inputWords];
            for(int j=0; j<inputWords; j++) {
                sum_arr[j] = pow(stod(inputData[0][j])-stod(inputData[1][j]),2)/stod(inputData[1][j]);
            }
            result << "𝜒² = ";
            for(int j = 0; j < inputWords; j++) result << setprecision(2) << ((j==0)?"":"+ ") << "(" << inputData[0][j] << "-" << inputData[1][j] << ")²/" << inputData[1][j] << " ";
            result<<endl;
            result << "𝜒² ≈ ";
            for(int i = 0; i < inputWords; i++){
                result << ((i==0)?"":"+ ") << sum_arr[i] << " ";
            };
            break;
        }
        default: break;
}
    return result.str();
}

void outputSolution (int index, const string& sign) {
    cout << endl << "The solution:"<<endl;
    ifstream my1("../Solutions inside code.txt");
    if (!my1.is_open()) cout << "Could not open the file!" << endl;
    string line;
    for (int i = 0;  i < index; i++) getline(my1, line);
    my1.close();
    size_t pos = 0;
    switch (index) {
        case 1: {
            double sigma = (searchParameter("population standard deviation").getValue())/searchParameter("sample size").getValue();
            double z = (searchParameter("margin of error").getValue())/sigma;
            double Prob = (1+erf(z/sqrt(2)))/2, prob = (1-erf(z/sqrt(2)))/2, deltap = Prob-prob;
            while ((pos = line.find("SIGMA")) != string::npos) {
                line.replace(pos++, 5, to_string(sigma));
            }
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(z));
            }
            while ((pos = line.find("PH")) != string::npos) {
                line.replace(pos, 2, to_string(Prob));
            }
            while ((pos = line.find("PL")) != string::npos) {
                line.replace(pos, 2, to_string(prob));
            }
            while ((pos = line.find("DP")) != string::npos) {
                line.replace(pos, 2, to_string(deltap));
            }
            break;
        }
        case 5: {
            double sigma = sqrt((searchParameter("population proportion").getValue())*(1-(searchParameter("population proportion").getValue()))/(searchParameter("sample size").getValue()));
            double z = (searchParameter("margin of error").getValue())*sqrt((searchParameter("sample size").getValue())/((searchParameter("population proportion").getValue())*(1-(searchParameter("population proportion").getValue()))));
            double Prob = (1+erf(z/sqrt(2)))/2, prob = (1-erf(z/sqrt(2)))/2, deltap = Prob-prob;
            while ((pos = line.find("SIGMA")) != string::npos) {
                line.replace(pos, 5, to_string(sigma));
            }
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(z));
            }
            while ((pos = line.find("PH")) != string::npos) {
                line.replace(pos, 2, to_string(Prob));
            }
            while ((pos = line.find("PL")) != string::npos) {
                line.replace(pos, 2, to_string(prob));
            }
            while ((pos = line.find("DP")) != string::npos) {
                line.replace(pos, 2, to_string(deltap));
            }
            break;
        };
        case 2: {
            int freedom = static_cast<int>(searchParameter("sample size").getValue()) - 1;
            double t = invstudenttdistribution(freedom,(searchParameter("confidence level").getValue()+1)/2,alglib::xparams{});
            double ME = t*searchParameter("sample standard deviation").getValue()/sqrt(searchParameter("sample size").getValue());
            double lowb = searchParameter("sample mean value").getValue()-ME, highb = searchParameter("sample mean value").getValue()+ME;
            while ((pos = line.find("DF")) != string::npos) {
                line.replace(pos, 2, to_string(freedom));
            }
            while ((pos = line.find("TTT")) != string::npos) {
                line.replace(pos, 3, to_string(t));
            }
            while ((pos = line.find("MEE")) != string::npos) {
                line.replace(pos, 3, to_string(ME));
            }
            while ((pos = line.find("LB")) != string::npos) {
                line.replace(pos, 2, to_string(lowb));
            }
            while ((pos = line.find("HB")) != string::npos) {
                line.replace(pos, 2, to_string(highb));
            }
            break;
        }
        case 6: {
            double z = sqrt(2)*inverf(searchParameter("confidence level").getValue(),alglib::xparams{});
            double ME = z*sqrt(searchParameter("sample proportion").getValue()*(1-searchParameter("sample proportion").getValue())/searchParameter("sample size").getValue());
            double lowb = searchParameter("sample proportion").getValue()-ME, highb = searchParameter("sample proportion").getValue()+ME;
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(z));
            }
            while ((pos = line.find("MEE")) != string::npos) {
                line.replace(pos, 3, to_string(ME));
            }
            while ((pos = line.find("LB")) != string::npos) {
                line.replace(pos, 2, to_string(lowb));
            }
            while ((pos = line.find("HB")) != string::npos) {
                line.replace(pos, 2, to_string(highb));
            }
            break;
        }
        case 3: {
            double z = sqrt(2)*inverf(searchParameter("confidence level").getValue(),alglib::xparams{});
            int n = ceil(pow(z*searchParameter("population standard deviation").getValue()/searchParameter("margin of error").getValue(),2));
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(z));
            }
            while ((pos = line.find("AMOUNT")) != string::npos) {
                line.replace(pos, 6, to_string(n));
            }
            break;
        }
        case 7: {
            double z = sqrt(2)*inverf(searchParameter("confidence level").getValue(),alglib::xparams{});
            double pp = searchParameter("sample proportion").getValue()*(1-searchParameter("sample proportion").getValue());
            int n = ceil(pp*pow(z/searchParameter("margin of error").getValue(),2));
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(z));
            }
            while ((pos = line.find("PP")) != string::npos) {
                line.replace(pos, 2, to_string(pp));
            }
            while ((pos = line.find("AMOUNT")) != string::npos) {
                line.replace(pos, 6, to_string(n));
            }
            break;
        }
        case 4: {
            ifstream my2("../Special Solutions.txt");
            if (!my2.is_open()) cout << "Could not open the file!" << endl;
            string newline;
            for (int i = 0;  i <= (((sign=="=")?0:((sign=="<=")?1:2))+(((*answer)==1)?3:0)); i++) getline(my2, newline);
            my2.close();
            line += newline;
            const double SE = sqrt((searchParameter("population proportion").getValue())*(1-(searchParameter("population proportion").getValue()))/searchParameter("sample size").getValue());
            const double zAct = (searchParameter("sample proportion").getValue()-searchParameter("population proportion").getValue())/SE;
            double z = sqrt(2)*inverf((sign=="=")?(searchParameter("confidence level").getValue()):2*searchParameter("confidence level").getValue()-1,alglib::xparams{});
            double conf = (sign=="=")?erf(abs(zAct)/sqrt(2)):((erf(abs(zAct)/sqrt(2))+1)/2);
            while ((pos = line.find("SIGN")) != string::npos) {
                line.replace(pos, 4, sign);
            }
            while ((pos = line.find("WORD")) != string::npos) {
                line.replace(pos, 4, (sign=="=")?"different":(sign=="<=")?"greater":"less");
            }
            while ((pos = line.find("DIFFERENT")) != string::npos) {
                line.replace(pos, 9, (sign=="=")?"≠":(sign=="<=")?">":"<");
            }
            while ((pos = line.find("SE")) != string::npos) {
                line.replace(pos, 2, to_string(SE));
            }
            while ((pos = line.find("ZZZ")) != string::npos) {
                line.replace(pos, 3, to_string(zAct));
            }
            while ((pos = line.find("Z0Z0")) != string::npos) {
                line.replace(pos, 4, to_string(z));
            }
            while ((pos = line.find("Z0ZZ")) != string::npos) {
                line.replace(pos, 4, to_string(abs(zAct)));
            }
            while ((pos = line.find("NEWCONF")) != string::npos) {
                line.replace(pos, 7, to_string(conf));
            }
            break;
        }
        case 8: {
            pos = 0;
            double chi_square = 0, chi_square_Act = 0, sum = 0;
            double expected[inputLines][inputWords] = {0}, marginal_bottom[inputWords] = {0}, marginal_left[inputLines] = {0};
            double degrees_of_freedom = (inputWords-1)*(inputLines-1);
            chi_square = 2*invincompletegammac(degrees_of_freedom/2,1-searchParameter("confidence level").getValue(),alglib::xparams{});
            for (int i=0; i<inputLines; i++) for (int j=0; j<inputWords; j++) {
                sum+=stod(inputData[i][j], &pos);
                marginal_bottom[j]+=stod(inputData[i][j], &pos);
                marginal_left[i]+=stod(inputData[i][j], &pos);
            }
            for (int i=0; i<inputLines; i++) for (int j=0; j<inputWords; j++) {
                expected[i][j] = marginal_bottom[j]*marginal_left[i]/sum;
                chi_square_Act+=pow(expected[i][j]-stod(inputData[i][j], &pos),2)/expected[i][j];
            }
            while ((pos = line.find("EXPECTEDOUTPUT")) != string::npos) {
                line.replace(pos, 14, expectedOutput(*expected,marginal_bottom,marginal_left,sum));
            }
            while ((pos = line.find("NEWTABLE")) != string::npos) {
                line.replace(pos, 8, newTable(*expected,static_cast<int>(stod(sign))));
            }
            while ((pos = line.find("CHISUM")) != string::npos) {
                line.replace(pos, 6, chiSum(*expected,0));
            }
            while ((pos = line.find("CHIACT")) != string::npos) {
                line.replace(pos, 6, to_string(chi_square_Act));
            }
            while ((pos = line.find("ROWS")) != string::npos) {
                line.replace(pos, 4, to_string(inputLines));
            }
            while ((pos = line.find("COLUMNS")) != string::npos) {
                line.replace(pos, 7, to_string(inputWords));
            }
            while ((pos = line.find("RM")) != string::npos) {
                line.replace(pos, 2, to_string(inputLines-1));
            }
            while ((pos = line.find("CM")) != string::npos) {
                line.replace(pos, 2, to_string(inputWords-1));
            }
            while ((pos = line.find("DF")) != string::npos) {
                line.replace(pos, 2, to_string(static_cast<int>(degrees_of_freedom)));
            }
            while ((pos = line.find("CHIEXP")) != string::npos) {
                line.replace(pos, 6, to_string(chi_square));
            }
            pos = line.find("SIGN"); line.replace(pos, 4, (chi_square_Act<=chi_square)?"lower":"higher");
            pos = line.find("FAILTO"); line.replace(pos, 6, (chi_square_Act<=chi_square)?"fail to ":"");
            pos = line.find("TO"); line.replace(pos, 2, (chi_square_Act<=chi_square)?"to ":"");
            break;
        }
        case 9: {
            bool flag=true;
            double expected_value = 0;
            pos = 0;
            for (int i = 0; i < inputWords; i++) {
                if (!isValidDouble(inputData[0][i])) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                ifstream my2("../Solutions inside code.txt");
                if (!my2.is_open()) cout << "Could not open the file!" << endl;
                for (int i = 0;  i < 10; i++) getline(my2, line);
                my2.close();
            }
            double chi_square = 0, chi_square_Act = 0, sum = 0;
            double degrees_of_freedom = inputWords-1;
            chi_square = 2*invincompletegammac(degrees_of_freedom/2,1-searchParameter("confidence level").getValue(),alglib::xparams{});
            if (flag) {
                for (int i=0; i<inputWords; i++) chi_square_Act+=pow((stod(inputData[0][i], &pos)-stod(inputData[1][i], &pos)),2)/stod(inputData[0][i], &pos);
            } else {
                for (int i=0; i<inputWords; i++) sum+=stod(inputData[1][i], &pos);
                expected_value = sum/inputWords;
                for (int i=0; i<inputWords; i++) chi_square_Act+=pow(expected_value-stod(inputData[1][i], &pos),2)/expected_value;
            }
            while ((pos = line.find("EXPECTED")) != string::npos) {
                line.replace(pos, 8, (flag)?"":("With "+to_string(static_cast<int>(sum))+" total of the second variable, the expected number of it in each square would be "+to_string(static_cast<int>(sum))+"/"+to_string(inputWords)+" = "+to_string(expected_value)+".\n"));
            }
            while ((pos = line.find("CHISUM")) != string::npos) {
                line.replace(pos, 6, chiSum(&expected_value,1+static_cast<int>(flag)));
            }
            while ((pos = line.find("CHIACT")) != string::npos) {
                line.replace(pos, 6, to_string(chi_square_Act));
            }
            while ((pos = line.find("WORDS")) != string::npos) {
                line.replace(pos, 5, to_string(inputWords));
            }
            while ((pos = line.find("DF")) != string::npos) {
                line.replace(pos, 2, to_string(static_cast<int>(degrees_of_freedom)));
            }
            while ((pos = line.find("CHIEXP")) != string::npos) {
                line.replace(pos, 6, to_string(chi_square));
            }
            pos = line.find("SIGN"); line.replace(pos, 4, (chi_square_Act<=chi_square)?"lower":"higher");
            pos = line.find("FAILTO"); line.replace(pos, 6, (chi_square_Act<=chi_square)?"fail to ":"");
            pos = line.find("TO"); line.replace(pos, 2, (chi_square_Act<=chi_square)?"to ":"");
            break;
        }
        default: break;
    };
    for (int i = 0; i < 8; i++) {
        string token = to_string(i) + "TYPE";
        while ((pos = line.find(token)) != string::npos) {
            ostringstream oss;
            oss << fixed << setprecision(2) << parameters[i].getValue();
            line.replace(pos, 5, oss.str());
        }
    }; pos = 0;
    while ((pos = line.find("\\n", ++pos)) != string::npos) {
        line.replace(pos, 2, "\n");
    }
    cout << line << endl;
};

void outputLast(HANDLE h, const int maxLines, const int words, const int W, const int selectedIndex, const int index) {
    int selectedChoice = 1;
    bool flag = true;
    while (flag) {
        displayMenu(h, selectedChoice, answerMenu, 3);
        printMatrix(inputData, maxLines, words, W*7+3, /*showRowLabels=*/ selectedIndex != 0);
        int key = _getch();  // Wait for key press
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) {  // Arrow keys return two values
            selectedChoice=selectedChoice+((selectedChoice==1)?1:-1);
        } else if (key == KEY_ENTER) {
            flag=false;
            selectedChoice--;
        } else if (key == KEY_ESC) {
            break;  // Exit program
        } else {
            // selectedIndex=selectedIndex+((selectedIndex==1)?1:-1);
        }
    }
    system("cls");
    printMatrix(inputData, maxLines, words, W*7+3, /*showRowLabels=*/ selectedIndex != 0);
    if (selectedChoice) {
        if((*answer)>1) cout << "The answer is:"<<endl<<"It (the second variable) is a "<<((*answer==2)?"bad":"good ")<<" model (for the first).";
        else cout << "The answer is:"<<endl<<"They are "<<((*answer==1)?"":"not ")<<"correlated.";
    }
    else {
        checkUserAnswer(index,answer);
    }
    getch();
    Option Steps[3] = {Option("Do you want to see the steps?:", 0, true, false),
                            Option("Yes", 1), Option("No", 1)};
    int newselectedIndex = navigateMenu(h, Steps, 3, 1);
    if (newselectedIndex==NO_LIMIT) return;
    if (newselectedIndex==1) {
        system("cls");
        printMatrix(inputData, maxLines, words, W*7+3, /*showRowLabels=*/ selectedIndex != 0);
        cout << endl;
        outputSolution(index, to_string(W*7+3));
        getch();
    };
}

void lastproblem(HANDLE h) {
    for(int r=0; r<MAX; ++r) for(int c=0; c<MAX; ++c) inputData[r][c].clear();
    Option options[3] = {Option("Select the subtype of your problem:", 0, true, false),
                        Option("2 properties of a population", 1), Option("2 variables", 1)};
    int selectedIndex = navigateMenu(h, options, 3, 1);
    if (selectedIndex==NO_LIMIT) return;
    selectedIndex--;
    system("cls");
    cout << "Enter the confidence level (should be between 0 and 1, preferably close to 1): ";
    double temp1;
    cin >> temp1;
    searchParameter("confidence level").setValue(temp1);
    cout << (selectedIndex?"Enter the values of the variables (separated with a space and no commas; each variable should be in a separate row, if both rows are quantitative, the first one must be actual and the second - expected.):":
        "Enter the numbers of individuals having certain combinations of variables (separated with a space and enter to get a new row; write no commas; write 'done' from new line when you are done):") << endl;
    int maxLines = selectedIndex?2:MAX;
    string temp[maxLines];
    int wordCount = 0;
    int maxLength = 0;
    cin.ignore();
    for (int totalLines = 0; totalLines<(maxLines)&&getline(cin, temp[totalLines]); totalLines++) {
        if(temp[totalLines]=="done") {maxLines=totalLines; break;}
        istringstream iss(temp[totalLines]);
        wordCount = 0;
        while(wordCount < MAX && iss >> inputData[totalLines][wordCount]) {
            maxLength = ((inputData[totalLines][wordCount].length() > maxLength) ? inputData[totalLines][wordCount].length() : maxLength);
            wordCount++;
        };
    }
    maxLength++;
    cout<<endl<<"Here is your input:"<<endl;
    printMatrix(inputData, maxLines, wordCount, maxLength, /*showRowLabels=*/ selectedIndex != 0);
    getch();
    inputLines = maxLines;
    inputWords = wordCount;
    int index = 8+selectedIndex;
    Solve(index, "");
    outputLast(h, maxLines, wordCount, floor(maxLength/7), selectedIndex, index);
}

void randomLast(HANDLE h) {
    for(int r=0; r<MAX; ++r) for(int c=0; c<MAX; ++c) inputData[r][c].clear();
    int selectedIndex = rand()%2;
    int maxLines = selectedIndex?2:(rand()%10+2);
    int words = rand()%10+2;
    int W = selectedIndex*rand()%2;
    if (selectedIndex==0) for(int i =0; i<maxLines; i++) for (int j = 0; j < words; j++) inputData[i][j]=to_string(rand()%40);
    else {
        if(W==0) {
            for (int j = 0; j < words; j++) inputData[0][j]=to_string(rand()%40);
        }
        else {
            for (int j = 0; j < words;j++) inputData[0][j]="Variable_"+to_string(1+j);
        }
        for (int j = 0; j < words; j++) inputData[1][j]=to_string(rand()%40);
    }
    printMatrix(inputData, maxLines, words, W*7+3, /*showRowLabels=*/ selectedIndex != 0);
    inputLines = maxLines;
    inputWords = words;
    int index = 8+selectedIndex;
    Solve(index, "");
    outputLast(h, maxLines, words, W, selectedIndex, index);
}

void OutputAnswer(const int N) {
    cout << "The answer is: ";
    switch (N) {
        case 1: case 3: case 5: case 7: cout << fixed << setprecision(4) << *answer << endl; break;
        case 2: case 6: cout << "(" << *answer << ", " << *(answer+1) << ")" << endl; break;
        case 4: cout<<((*answer==1)?"We reject the null hypothesis and can confirm the claim.":"We fail to reject the null hypothesis and can not confirm the claim."); break;
        default: break;
    }
}

void parameterInput (int index, Option menu[], HANDLE Console) {
    index = static_cast<int>(static_cast<unsigned char>(menu[index-2].getName().back()-'0'));
    system("cls");
    int selectedIndex = 1;
    Option signs[4] = {Option("Select how you want the theorised percentage of individuals with a specific feature in the population to compare with the one you will provide:", 0, true, false),
                            Option("=", 1), Option(">=", 1), Option("<=", 1)};
    if(1<=index && index<=3) {
        bool flag = true;
        Option options[3] = {Option("Select the subtype of your problem:", 0, true, false),
                            Option("Feature", 1), Option("Proportion", 1)};
        while (flag) {
            displayMenu(Console, selectedIndex, options, 3);
            int key = _getch();  // Wait for key press
            if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) {  // Arrow keys return two values
                selectedIndex=selectedIndex+((selectedIndex==1)?1:-1);
            } else if (key == KEY_ENTER) {
                flag=false;
                const int temp =  index;
                index = index + ((selectedIndex==1)?0:4);
                selectedIndex = temp;
            } else if (key == KEY_ESC) {
                break;  // Exit program
            } else {
                // selectedIndex=selectedIndex+((selectedIndex==1)?1:-1);
            }
        }
    } else if(index == 4) {
        bool flag = true;
        selectedIndex = navigateMenu(Console, signs, 4, 1);
        if (selectedIndex==NO_LIMIT) return;
    } else if(index==5) {
        lastproblem(Console);
        return;
    }
    system("cls");
    SetConsoleTextAttribute(Console, Selected);
    cout << "Enter the parameters:" << endl << endl;
    SetConsoleTextAttribute(Console, Normal);
    for (int i = 1; i <= 8; i++) {
        if (parameters[i-1].isAccounted(index-1)) {
            double temp;
            do {
                cout << "Enter the " <<  *parameters[i-1].getName() << parameters[i-1].getLimits() << ": ";
                cin >> temp;
            } while ((temp<=parameters[i-1].getLower()&&parameters[i-1].getLower()!=-1)||(temp>=parameters[i-1].getUpper()&&parameters[i-1].getUpper()!=-1));
            parameters[i-1].setValue(temp);
        }
    }
    Solve(index, signs[selectedIndex].getName());
    const int check = NavigateMenuAndOutputProblem(Console, index, signs[selectedIndex].getName());
    system("cls");
    outputProblem(index, signs[selectedIndex].getName());
    cout << endl;
    if(check==1){
        checkUserAnswer(index, answer);
    } else OutputAnswer(index);
    if (index==4) {SetConsoleOutputCP(CP_UTF8); cout << endl << "And the null hypothesis is: p"; switch (selectedIndex){case 1:cout<<"≠";break;case 2:cout<<"<=";break;case 3:cout<<">=";break;default:break;}; cout << searchParameter("population proportion").getValue() << ";" << endl;}
    getch();
    Option Steps[3] = {Option("Do you want to see the steps?:", 0, true, false),
                            Option("Yes", 1), Option("No", 1)};
    bool flag = true;
    int newselectedIndex = 1;
    while (flag) {
        displayMenu(Console, newselectedIndex, Steps, 3);
        cout << endl << endl;
        OutputAnswer(index);
        int key = _getch();  // Wait for key press
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) newselectedIndex=newselectedIndex+((newselectedIndex==1)?1:-1);
        else if (key == KEY_ENTER) flag=false;
        else if (key == KEY_ESC) break;  // Exit program
        else /*selectedIndex=selectedIndex+((selectedIndex==1)?1:-1)*/;
    }
    if (newselectedIndex==1) {
        system("cls");
        outputProblem(index, signs[selectedIndex].getName()); cout << endl;
        outputSolution(index, signs[selectedIndex].getName());
        getch();
    };
}

void randomInput(int index, Option menu[], HANDLE Console) {
    Parameter::generateVaue(parameters, 8);
    if (index==NO_LIMIT) index=rand()%5+1;
    else index = static_cast<int>(static_cast<unsigned char>(menu[index-3].getName().back()-'0'));
    system("cls");
    int selectedIndex = 1;
    Option signs[4] = {Option("Select how you want the theorised percentage of individuals with a specific feature in the population to compare with the one you will provide:", 0, true, false),
                            Option("=", 1), Option(">=", 1), Option("<=", 1)};
    if(1<=index && index<=3) {
        index = index + ((rand()%2==1)?0:4);
    } else if(index==5) {
        randomLast(Console);
        return;
    }
    system("cls");
    selectedIndex = rand()%3+1;
    Solve(index, signs[selectedIndex].getName());
    const int check = NavigateMenuAndOutputProblem(Console, index, signs[selectedIndex].getName());
    system("cls");
    outputProblem(index, signs[selectedIndex].getName());
    cout << endl;
    if(check==1){
        checkUserAnswer(index, answer);
    } else OutputAnswer(index);
    if (index==4) {SetConsoleOutputCP(CP_UTF8); cout << endl << "And the null hypothesis is: p"; switch (selectedIndex){case 1:cout<<"≠";break;case 2:cout<<"<=";break;case 3:cout<<">=";break;default:break;}; cout << searchParameter("population proportion").getValue() << ";" << endl;}
    getch();
    Option Steps[3] = {Option("Do you want to see the steps?:", 0, true, false),
                            Option("Yes", 1), Option("No", 1)};
    bool flag = true;
    int newselectedIndex = 1;
    while (flag) {
        displayMenu(Console, newselectedIndex, Steps, 3);
        cout << endl << endl;
        OutputAnswer(index);
        int key = _getch();  // Wait for key press
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) newselectedIndex=newselectedIndex+((newselectedIndex==1)?1:-1);
        else if (key == KEY_ENTER) flag=false;
        else if (key == KEY_ESC) break;  // Exit program
        else /*selectedIndex=selectedIndex+((selectedIndex==1)?1:-1)*/;
    }
    if (newselectedIndex==1) {
        system("cls");
        outputProblem(index, signs[selectedIndex].getName()); cout << endl;
        outputSolution(index, signs[selectedIndex].getName());
        getch();
    };
}


int main() {
    srand(time(nullptr));
    SetConsoleOutputCP(CP_UTF8);
    int maxmenuSize = 30;
    Option menu[maxmenuSize] = {Option("Select the problem manually"),
                                Option("Select the problem type", 1, false, false),
                                Option("Random Selection", 1, false),
                                Option("Type 1", 1, false),
                                Option("Type 2", 1, false),
                                Option("Type 3", 1, false),
                                Option("Type 4", 1, false),
                                Option("Type 5", 1, false),
                                Option("Get a random problem"),
                                Option("Help"),
                                Option("About author"),
                                Option("Exit")};
    HANDLE Console = GetStdHandle(STD_OUTPUT_HANDLE);
    setConsoleHeightLimit(Console);
    int selectedIndex = 0;  // Start with the first option
    while (true) {
        displayMenu(Console, selectedIndex, menu, maxmenuSize);
        int key = _getch();  // Wait for key press
        if (key == KEY_ARROW_UP || key == KEY_ARROW_DOWN) {  // Arrow keys return two values
            do{
                selectedIndex=(selectedIndex+((key == KEY_ARROW_DOWN)?(1):(maxmenuSize-1)))%(maxmenuSize); // Move
            }while (!((menu[selectedIndex].isVisible())&&(menu[selectedIndex].isSelectable())));
        } else if (key == KEY_ENTER) {
            if (menu[selectedIndex].getName()=="Exit") break;  // Exit when selecting "Exit"
            if (menu[selectedIndex].getName()=="Help") showHelp();
            if (menu[selectedIndex].getName()=="About author") showAuthor();
            if (menu[selectedIndex].getName()=="Select the problem manually") toggleManual(selectedIndex, maxmenuSize, menu); //Toggling the options
            if (menu[selectedIndex].getName()=="Random Selection") while (menu[selectedIndex].getName().compare(0, 4, "Type")!=0) selectedIndex=(rand())%(maxmenuSize);
            if (menu[selectedIndex].getName()=="Get a random problem") randomInput(NO_LIMIT, menu, Console);
            if((menu[selectedIndex].getName().compare(0, 4, "Type") == 0)) {
                if(menu[selectedIndex+1].getOffset()!=2) Expand(selectedIndex, maxmenuSize, menu);
                else for(int i=1; i<=3; i++) menu[selectedIndex+i].toggleVisibility();
            }
            if (menu[selectedIndex].getName()=="Get a random problem") {
            };
            if (menu[selectedIndex].getName()=="Manually") {
                parameterInput(selectedIndex, menu, Console);
            };
            if (menu[selectedIndex].getName()=="Randomly") {
                randomInput(selectedIndex, menu, Console);
            };
        } else if (key == KEY_ESC) {
            break;  // Exit program
        }
    }
    return 0;
}