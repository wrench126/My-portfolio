from datetime import datetime

def main_manu(balance, main_pin) :
 while True :  
   print("""
      Welcome User in EG Bank ATM.
      *****************************
      Please choose youer service:
        1. Check youer Balance.
        2. Deposit.
        3. Withdraw.
        4. Change PIN.
        5. Exit.
         """)
   choice = input ("Enter youer choice :")
   if   choice == '1' :
     print (f"Youer balance is : {balance}")
   elif choice == '2' :
     dep_amount = float(input("Enter amount of deposit :"))
     balance = balance + dep_amount
     current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
     print(f"A deposit has been made : {dep_amount} at {current_time}\n Your New Balance is: {balance}")
   elif choice == '3' :
    while True:
        with_amount = float(input("Enter amount of Withdraw : "))
        if with_amount < int(balance):
            balance = balance - with_amount
            current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"A withdrawal was made : {with_amount} at {current_time}\n Your New Balance is: {balance}")
            break
        else:
            print("Insufficient Balance")
   elif choice == '4' :
              old_pin = int(input("Enter old PIN: "))
              new_pin = int(input("Enter new PIN: "))
              if old_pin == main_pin:
                main_pin = new_pin
                print("PIN changed successfully.")
                x = check_pass(main_pin)
                if x == True:
                   print(f"\n\n\nPIN has been changed To {main_pin}\n\n\n")
                   main_manu(balance, main_pin)
                break
              else:
                print("Incorrect old PIN.")
   elif choice == '5' :
      print("Thank you for using EG Bank ATM. Goodbye!")
      break
   else :
      return "Invalid Choice , please try again !"

def create_new_pin():
    new_pin = int(input("Create PIN for you account : "))
    return(new_pin)


def check_pass(main_pin) :
 while True :
  print("Please log in with your new PIN.")
  entered_pin = int(input("Enter PIN Code :"))
  if main_pin == entered_pin :
      print("Correct PIN !")
      return True
  else :
   print ("Incorrect PIN !")


main_pin = create_new_pin()
balance = 1000000

if check_pass(main_pin) == True:
   main_manu (balance, main_pin)
else:
   quit