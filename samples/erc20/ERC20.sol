pragma solidity ^0.4.0;

/*contract ERC20_Interface {
  function totalSupply() public constant returns (uint256 supply);
  function balanceOf(address _owner) public constant returns (uint256 balance);
  function transfer(address _to, uint256 _value) public returns (bool success);
  function transferFrom(address _from, address _to, uint256 _value) public returns (bool success);
  function approve(address _spender, uint256 _value) public returns (bool success);
  function allowance(address _owner, address _spender) public constant returns (uint256 remaining);

  event Transfer(address indexed _from, address indexed _to, uint256 _value);
  event Approval(address indexed _owner, address indexed _spender, uint256 _value);
}*/

contract ERC20Token {
  uint256 supply;
  mapping (address => uint256) balances;
  mapping (address => mapping (address => uint256)) allowances;

  event Transfer(address indexed _from, address indexed _to, uint256 _value);
  event Approval(address indexed _owner, address indexed _spender, uint256 _value);
  
  constructor(uint256 s) public
  {
    supply = s;
    balances[msg.sender] = supply;
  }
  
  // Get the total token supply
  function totalSupply() public constant returns (uint256)
  {
    return supply;
  }
  
  // Get the account balance of another account with address _owner
  function balanceOf(address _owner) public constant returns (uint256)
  {
    return balances[_owner];
  }
  
  // Send _value amount of tokens to address _to
  function transfer(address _to, uint256 _value) public returns (bool)
  {
    if (balances[msg.sender] >= _value)
    {
      balances[msg.sender] -= _value;
      balances[_to] += _value;
      emit Transfer(msg.sender, _to, _value);
      return true;
    }
    else
    {
      return false;
    }
  }
  
  // Send _value amount of tokens from address _from to address _to
  function transferFrom(address _from, address _to, uint256 _value) public returns (bool)
  {
    if (balances[_from] >= _value && allowances[_from][msg.sender] >= _value)
    {
      balances[_from] -= _value;
      balances[_to] += _value;
      allowances[_from][msg.sender] -= _value;
      emit Transfer(msg.sender, _to, _value);
      return true;
    }
    else
    {
      return false;
    }
  }
  
  // Allow _spender to withdraw from your account, multiple times, up to the
  // _value amount. If this function is called again it overwrites the current
  // allowance with _value
  function approve(address _spender, uint256 _value) public returns (bool)
  {
    // We permit allowances to be larger than balances
    allowances[msg.sender][_spender] = _value;
    emit Approval(msg.sender, _spender, _value);
    return true;
  }
  
  // Returns the amount which _spender is still allowed to withdraw from _owner
  function allowance(address _owner, address _spender) public constant returns (uint256)
  {
    return allowances[_owner][_spender];
  }
}  
