class ATM :
    def __init__(self, pin  , balance )  :
        self.balance = balance
        self.pin = pin 
    def check_pin (self) :
        return self.pin 
    def Check_balance (self) :
        return self.balance    
    def Deposit (self, amount) :
        self.balance += amount 
        return self.balance
    def With_draw (self , amount2) :
        if amount2 > self.balance :
            return "Insufficient funds!"
        else :
            self.balance -= amount2
            return self.balance
    def change_password(self, old_pin, new_pin):
        if self.pin == old_pin:
            self.pin = new_pin
            return "PIN changed successfully."
        else:
            return "Incorrect old PIN"    
               



    
         
      