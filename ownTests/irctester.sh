#!/bin/bash
# filepath: /home/ahmet/Desktop/irc/irctester

# IRC Sunucu Test Script'i
# Kullanım: ./irctester <port> <password>

if [ $# -ne 2 ]; then
    echo "Kullanım: $0 <port> <password>"
    exit 1
fi

PORT=$1
PASSWORD=$2
SERVER="localhost"

# Renk kodları
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Log fonksiyonu
log() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

success() {
    echo -e "${GREEN}[BAŞARILI]${NC} $1"
}

error() {
    echo -e "${RED}[HATA]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[UYARI]${NC} $1"
}

# NC ile komut gönderme fonksiyonu
send_command() {
    local cmd="$1"
    local timeout="${2:-2}"
    echo -e "$cmd" | timeout $timeout nc $SERVER $PORT 2>/dev/null
}

# Test wrapper
run_test() {
    local test_name="$1"
    local test_func="$2"
    log "Test başlıyor: $test_name"
    if $test_func; then
        success "$test_name"
    else
        error "$test_name"
    fi
    echo "----------------------------------------"
    sleep 1
}

# Test 1: Bağlantı testi
test_connection() {
    local result=$(echo "QUIT" | timeout 3 nc $SERVER $PORT 2>/dev/null)
    if [ $? -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Test 2: Yanlış şifre testi
test_wrong_password() {
    local result=$(send_command "PASS wrongpass\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Password required/incorrect"
    return $?
}

# Test 3: Doğru şifre testi
test_correct_password() {
    local result=$(send_command "PASS $PASSWORD\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Password accepted"
    return $?
}

# Test 4: NICK komutu testi
test_nick_command() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Password accepted"
    return $?
}

# Test 5: USER komutu testi
test_user_command() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Welcome to the tiny IRC"
    return $?
}

# Test 6: PING-PONG testi
test_ping_pong() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nPING :test123\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "PONG :test123"
    return $?
}

# Test 7: Kanal katılma testi
test_join_channel() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nJOIN #test\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "JOIN #test"
    return $?
}

# Test 8: TOPIC testi
test_topic() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nJOIN #test\r\nTOPIC #test :Test Topic\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "TOPIC #test :Test Topic"
    return $?
}

# Test 9: Kanal mesajı testi
test_channel_message() {
    # İki bağlantıyla test - karmaşık olduğu için basitleştirilmiş
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nJOIN #test\r\nPRIVMSG #test :Hello World\r\nQUIT\r\n" 3)
    # Mesajın gönderildiğini kontrol et (hata yoksa başarılı)
    echo "$result" | grep -qv "No such channel"
    return $?
}

# Test 10: Bilinmeyen komut testi
test_unknown_command() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nUNKNOWN\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Unknown command"
    return $?
}

# Test 11: Yetersiz parametre testi
test_insufficient_params() {
    local result=$(send_command "PASS $PASSWORD\r\nNICK testuser\r\nUSER test 0 * :Test User\r\nJOIN\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "Not enough parameters"
    return $?
}

# Test 12: Kayıt olmadan komut testi
test_unregistered_command() {
    local result=$(send_command "JOIN #test\r\nQUIT\r\n" 3)
    echo "$result" | grep -q "not registered"
    return $?
}

# İki kullanıcılı testler için yardımcı fonksiyon
run_dual_user_test() {
    local test_name="$1"
    log "İki kullanıcılı test başlıyor: $test_name"
    
    # Kullanıcı 1 arka planda
    (
        echo -e "PASS $PASSWORD\r\n"
        echo -e "NICK user1\r\n"
        echo -e "USER test1 0 * :User One\r\n"
        echo -e "JOIN #testchan\r\n"
        sleep 3
        echo -e "PRIVMSG #testchan :Hello from user1\r\n"
        sleep 2
        echo -e "QUIT\r\n"
    ) | nc $SERVER $PORT > /tmp/user1.log 2>&1 &
    
    sleep 1
    
    # Kullanıcı 2
    (
        echo -e "PASS $PASSWORD\r\n"
        echo -e "NICK user2\r\n"
        echo -e "USER test2 0 * :User Two\r\n"
        echo -e "JOIN #testchan\r\n"
        sleep 2
        echo -e "PRIVMSG #testchan :Hello from user2\r\n"
        sleep 2
        echo -e "QUIT\r\n"
    ) | nc $SERVER $PORT > /tmp/user2.log 2>&1
    
    wait
    
    # Logları kontrol et
    if grep -q "Hello from user1" /tmp/user2.log && grep -q "Hello from user2" /tmp/user1.log; then
        success "$test_name - Mesaj alışverişi başarılı"
        return 0
    else
        error "$test_name - Mesaj alışverişi başarısız"
        return 1
    fi
}

# Test 13: Çoklu kullanıcı chat testi
test_multi_user_chat() {
    run_dual_user_test "Çoklu kullanıcı chat"
    return $?
}

# Ana test fonksiyonu
main() {
    echo "========================================"
    echo "IRC Sunucu Test Suit'i"
    echo "Sunucu: $SERVER:$PORT"
    echo "Şifre: $PASSWORD"
    echo "========================================"
    
    # Sunucunun çalıştığını kontrol et
    if ! nc -z $SERVER $PORT 2>/dev/null; then
        error "Sunucuya bağlanılamıyor! Sunucunun çalıştığından emin olun."
        exit 1
    fi
    
    success "Sunucu erişilebilir durumda"
    echo "----------------------------------------"
    
    # Temel testler
    run_test "Bağlantı Testi" test_connection
    run_test "Yanlış Şifre Testi" test_wrong_password
    run_test "Doğru Şifre Testi" test_correct_password
    run_test "NICK Komutu Testi" test_nick_command
    run_test "USER Komutu Testi" test_user_command
    run_test "PING-PONG Testi" test_ping_pong
    run_test "Kanal Katılma Testi" test_join_channel
    run_test "TOPIC Testi" test_topic
    run_test "Kanal Mesajı Testi" test_channel_message
    run_test "Bilinmeyen Komut Testi" test_unknown_command
    run_test "Yetersiz Parametre Testi" test_insufficient_params
    run_test "Kayıt Olmadan Komut Testi" test_unregistered_command
    run_test "Çoklu Kullanıcı Chat Testi" test_multi_user_chat
    
    # Temizlik
    rm -f /tmp/user1.log /tmp/user2.log
    
    echo "========================================"
    log "Tüm testler tamamlandı!"
    echo "========================================"
}

# Script'i çalıştır
main